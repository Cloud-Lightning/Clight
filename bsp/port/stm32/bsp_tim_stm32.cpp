#include "bsp_tim.h"

#include <cstdint>

extern "C" {
#include "tim.h"
}

#ifndef BSP_STM32_FRAMEWORK_OWNS_TIM_IRQ_HANDLERS
#define BSP_STM32_FRAMEWORK_OWNS_TIM_IRQ_HANDLERS 0
#endif

extern "C" void Default_Handler(void);
extern "C" void bsp_stm32_pwm_period_callback(TIM_HandleTypeDef *htim) __attribute__((weak));

typedef struct {
    TIM_TypeDef *base;
    IRQn_Type irq;
    uint32_t features;
    bool has_pinmux;
    TIM_HandleTypeDef *handle;
    void (*init_fn)(void);
    bool internal_handle;
    bool initialized;
    bool oneshot;
    bsp_tim_event_callback_t callback;
    void *callback_arg;
} tim_map_t;

static TIM_HandleTypeDef s_htim_adc_trigger;

static void tim_init_fn_main()
{
    MX_TIM12_Init();
}

static void tim_init_fn_adc_trigger()
{
    __HAL_RCC_TIM2_CLK_ENABLE();
    s_htim_adc_trigger.Instance = TIM2;
    s_htim_adc_trigger.Init.Prescaler = BSP_TIM_MAIN_DEFAULT_PRESCALER;
    s_htim_adc_trigger.Init.CounterMode = TIM_COUNTERMODE_UP;
    s_htim_adc_trigger.Init.Period = 999U;
    s_htim_adc_trigger.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    s_htim_adc_trigger.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
}

#define BSP_TIM_MAP_ITEM(name, base, irq, features, has_pinmux) { base, irq, features, (has_pinmux) != 0U, nullptr, nullptr, false, false, false, nullptr, nullptr },
static tim_map_t s_tim_map[BSP_TIM_MAX] = {
    BSP_TIM_LIST(BSP_TIM_MAP_ITEM)
};
#undef BSP_TIM_MAP_ITEM

static void attach_tim_bindings()
{
    static bool attached = false;
    if (attached) {
        return;
    }

    s_tim_map[BSP_TIM_MAIN].handle = &htim12;
    s_tim_map[BSP_TIM_MAIN].init_fn = tim_init_fn_main;
    s_tim_map[BSP_TIM_ADC_TRIGGER].handle = &s_htim_adc_trigger;
    s_tim_map[BSP_TIM_ADC_TRIGGER].init_fn = tim_init_fn_adc_trigger;
    s_tim_map[BSP_TIM_ADC_TRIGGER].internal_handle = true;
    attached = true;
}

static int32_t ensure_tim_initialized(bsp_tim_instance_t timer)
{
    if (timer >= BSP_TIM_MAX) {
        return BSP_ERROR_PARAM;
    }

    attach_tim_bindings();
    auto &map = s_tim_map[timer];
    if (!map.has_pinmux) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (map.handle == nullptr || map.init_fn == nullptr) {
        return BSP_ERROR_PARAM;
    }
    if (!map.initialized) {
        map.init_fn();
        map.initialized = true;
    }
    return BSP_OK;
}

static bool tim_supports_mode(const tim_map_t &map, bsp_tim_mode_t mode)
{
    switch (mode) {
    case BSP_TIM_MODE_PERIODIC:
        return (map.features & BSP_TIM_FEATURE_PERIODIC) != 0U;
    case BSP_TIM_MODE_ONESHOT:
        return (map.features & BSP_TIM_FEATURE_ONESHOT) != 0U;
    case BSP_TIM_MODE_CAPTURE_RISING:
        return (map.features & BSP_TIM_FEATURE_CAPTURE) != 0U;
    case BSP_TIM_MODE_COMPARE:
        return (map.features & BSP_TIM_FEATURE_COMPARE) != 0U;
    default:
        return false;
    }
}

static bool irq_has_real_handler(IRQn_Type irq)
{
    if (irq < 0) {
        return false;
    }

    const auto vector_index = static_cast<std::uint32_t>(irq) + 16U;
    const auto *vector_table = reinterpret_cast<const std::uintptr_t *>(SCB->VTOR);
    const auto handler = vector_table[vector_index] & ~static_cast<std::uintptr_t>(1U);
    const auto default_handler = reinterpret_cast<std::uintptr_t>(&Default_Handler) & ~static_cast<std::uintptr_t>(1U);
    return (handler != 0U) && (handler != default_handler);
}

int32_t bsp_tim_init(bsp_tim_instance_t timer, bsp_tim_mode_t mode, uint32_t period_ticks)
{
    const auto init_status = ensure_tim_initialized(timer);
    if (init_status != BSP_OK) {
        return init_status;
    }

    auto &map = s_tim_map[timer];
    if (!tim_supports_mode(map, mode)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if ((mode == BSP_TIM_MODE_CAPTURE_RISING) || (mode == BSP_TIM_MODE_COMPARE)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    map.oneshot = (mode == BSP_TIM_MODE_ONESHOT);
    if (period_ticks > 0U) {
        map.handle->Init.Prescaler = BSP_TIM_MAIN_DEFAULT_PRESCALER;
        map.handle->Init.Period = period_ticks - 1U;
        if (HAL_TIM_Base_Init(map.handle) != HAL_OK) {
            return BSP_ERROR;
        }
    }

    __HAL_TIM_SET_COUNTER(map.handle, 0U);
    return BSP_OK;
}

static uint32_t trigger_output_to_hal(bsp_tim_trigger_output_t output)
{
    switch (output) {
    case BSP_TIM_TRIGGER_OUTPUT_UPDATE:
        return TIM_TRGO_UPDATE;
    case BSP_TIM_TRIGGER_OUTPUT_OC1:
        return TIM_TRGO_OC1;
    case BSP_TIM_TRIGGER_OUTPUT_OC2:
        return TIM_TRGO_OC2REF;
    case BSP_TIM_TRIGGER_OUTPUT_OC3:
        return TIM_TRGO_OC3REF;
    case BSP_TIM_TRIGGER_OUTPUT_OC4:
        return TIM_TRGO_OC4REF;
    case BSP_TIM_TRIGGER_OUTPUT_DISABLE:
    default:
        return TIM_TRGO_RESET;
    }
}

static uint32_t timer_source_clock_hz(TIM_TypeDef *timer)
{
    RCC_ClkInitTypeDef clk_config = {};
    uint32_t flash_latency = 0U;
    HAL_RCC_GetClockConfig(&clk_config, &flash_latency);

#if defined(TIM1) || defined(TIM8) || defined(TIM15) || defined(TIM16) || defined(TIM17)
    const bool apb2_timer =
#if defined(TIM1)
        (timer == TIM1) ||
#endif
#if defined(TIM8)
        (timer == TIM8) ||
#endif
#if defined(TIM15)
        (timer == TIM15) ||
#endif
#if defined(TIM16)
        (timer == TIM16) ||
#endif
#if defined(TIM17)
        (timer == TIM17) ||
#endif
        false;
    if (apb2_timer) {
        const uint32_t pclk = HAL_RCC_GetPCLK2Freq();
        return (clk_config.APB2CLKDivider == RCC_HCLK_DIV1) ? pclk : (pclk * 2U);
    }
#else
    (void) timer;
#endif

    const uint32_t pclk = HAL_RCC_GetPCLK1Freq();
    return (clk_config.APB1CLKDivider == RCC_HCLK_DIV1) ? pclk : (pclk * 2U);
}

int32_t bsp_tim_start(bsp_tim_instance_t timer)
{
    const auto init_status = ensure_tim_initialized(timer);
    if (init_status != BSP_OK) {
        return init_status;
    }

    auto &map = s_tim_map[timer];
    const bool use_interrupt = map.oneshot || (map.callback != nullptr);
    if (use_interrupt) {
        if (!irq_has_real_handler(map.irq)) {
            return BSP_ERROR_UNSUPPORTED;
        }
        HAL_NVIC_SetPriority(map.irq, BSP_IRQ_PRIORITY_DEFAULT, 0U);
        HAL_NVIC_EnableIRQ(map.irq);
    }
    const auto status = use_interrupt ? HAL_TIM_Base_Start_IT(map.handle) : HAL_TIM_Base_Start(map.handle);
    return (status == HAL_OK) ? BSP_OK : BSP_ERROR_BUSY;
}

int32_t bsp_tim_stop(bsp_tim_instance_t timer)
{
    const auto init_status = ensure_tim_initialized(timer);
    if (init_status != BSP_OK) {
        return init_status;
    }

    auto &map = s_tim_map[timer];
    HAL_TIM_Base_Stop_IT(map.handle);
    HAL_TIM_Base_Stop(map.handle);
    return BSP_OK;
}

int32_t bsp_tim_get_counter(bsp_tim_instance_t timer, uint32_t *value)
{
    if ((timer >= BSP_TIM_MAX) || (value == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    const auto init_status = ensure_tim_initialized(timer);
    if (init_status != BSP_OK) {
        return init_status;
    }

    *value = __HAL_TIM_GET_COUNTER(s_tim_map[timer].handle);
    return BSP_OK;
}

int32_t bsp_tim_get_capture(bsp_tim_instance_t timer, uint32_t *value)
{
    (void) timer;
    (void) value;
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_tim_set_compare(bsp_tim_instance_t timer, uint32_t compare_ticks)
{
    (void) timer;
    (void) compare_ticks;
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_tim_get_clock(bsp_tim_instance_t timer, uint32_t *freq_hz)
{
    if ((timer >= BSP_TIM_MAX) || (freq_hz == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    const auto init_status = ensure_tim_initialized(timer);
    if (init_status != BSP_OK) {
        return init_status;
    }

    *freq_hz = timer_source_clock_hz(s_tim_map[timer].base);
    return BSP_OK;
}

int32_t bsp_tim_config_trigger_output(bsp_tim_instance_t timer, bsp_tim_trigger_output_t output)
{
    if (timer >= BSP_TIM_MAX) {
        return BSP_ERROR_PARAM;
    }
    const auto init_status = ensure_tim_initialized(timer);
    if (init_status != BSP_OK) {
        return init_status;
    }

    auto &map = s_tim_map[timer];
    if ((map.features & BSP_TIM_FEATURE_TRIGGER_OUTPUT) == 0U) {
        return BSP_ERROR_UNSUPPORTED;
    }

    TIM_MasterConfigTypeDef master = {};
    master.MasterOutputTrigger = trigger_output_to_hal(output);
    master.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    return (HAL_TIMEx_MasterConfigSynchronization(map.handle, &master) == HAL_OK) ? BSP_OK : BSP_ERROR;
}

int32_t bsp_tim_register_event_callback(bsp_tim_instance_t timer, bsp_tim_event_callback_t callback, void *arg)
{
    if (timer >= BSP_TIM_MAX) {
        return BSP_ERROR_PARAM;
    }

    attach_tim_bindings();
    s_tim_map[timer].callback = callback;
    s_tim_map[timer].callback_arg = arg;
    return BSP_OK;
}

extern "C" void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM4) {
        HAL_IncTick();
        return;
    }

    attach_tim_bindings();
    for (std::uint32_t index = 0; index < BSP_TIM_MAX; ++index) {
        auto &map = s_tim_map[index];
        if (map.handle == htim) {
            if (map.callback != nullptr) {
                map.callback(BSP_TIMER_EVENT_ELAPSED,
                             static_cast<uint32_t>(__HAL_TIM_GET_AUTORELOAD(htim) + 1U),
                             map.callback_arg);
            }
            if (map.oneshot) {
                HAL_TIM_Base_Stop_IT(htim);
            }
        }
    }
    if (bsp_stm32_pwm_period_callback != nullptr) {
        bsp_stm32_pwm_period_callback(htim);
    }
}

#if BSP_STM32_FRAMEWORK_OWNS_TIM_IRQ_HANDLERS
/*
 * The framework enables timer NVIC lines when Timer::start() needs interrupt
 * mode. If CubeMX owns these handlers, set
 * BSP_STM32_FRAMEWORK_OWNS_TIM_IRQ_HANDLERS=0 and forward the IRQs from
 * stm32h7xx_it.c to HAL_TIM_IRQHandler().
 */
extern "C" void TIM2_IRQHandler(void)
{
    attach_tim_bindings();
    HAL_TIM_IRQHandler(s_tim_map[BSP_TIM_ADC_TRIGGER].handle);
}

extern "C" void TIM8_BRK_TIM12_IRQHandler(void)
{
    attach_tim_bindings();
    HAL_TIM_IRQHandler(s_tim_map[BSP_TIM_MAIN].handle);
}
#endif

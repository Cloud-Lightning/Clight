#include "bsp_pwm.h"

#include <cstdint>

extern "C" {
#include "main.h"
#include "tim.h"
}

namespace {

constexpr std::uint8_t k_tim_channel_count = 4U;

typedef struct {
    uint32_t backend;
    TIM_TypeDef *base;
    IRQn_Type irq;
    bool has_pinmux;
    uint8_t channel_base;
    uint32_t features;
    TIM_HandleTypeDef owned_handle;
    TIM_HandleTypeDef *handle;
    void (*init_fn)(void);
    bool initialized;
    uint32_t frequency_hz;
    bsp_pwm_align_t align;
    bool output_enabled[k_tim_channel_count];
    bool pair_enabled[k_tim_channel_count];
    bool trigger_compare_configured;
    uint32_t trigger_compare_ticks;
    bool running;
    bsp_pwm_event_callback_t callback;
    void *callback_arg;
} pwm_map_t;

#define BSP_PWM_MAP_ITEM(name, backend, base, irq, clk, channel_base, has_pinmux, features) \
    { backend, base, irq, (has_pinmux) != 0U, static_cast<uint8_t>(channel_base), static_cast<uint32_t>(features), {}, nullptr, nullptr, false, 0U, BSP_PWM_ALIGN_EDGE, {}, {}, false, 0U, false, nullptr, nullptr },
static pwm_map_t s_pwm_map[BSP_PWM_MAX] = {
    BSP_PWM_LIST(BSP_PWM_MAP_ITEM)
};
#undef BSP_PWM_MAP_ITEM

static void attach_pwm_bindings()
{
    static bool attached = false;
    if (attached) {
        return;
    }

    s_pwm_map[BSP_PWM_MAIN].handle = &s_pwm_map[BSP_PWM_MAIN].owned_handle;

    s_pwm_map[BSP_PWM_BUZZER].handle = &htim12;
    s_pwm_map[BSP_PWM_BUZZER].init_fn = MX_TIM12_Init;

    attached = true;
}

static bool pwm_supports(const pwm_map_t &map, uint32_t feature)
{
    return (map.features & feature) != 0U;
}

static bool logical_channel_to_hal(const pwm_map_t &map, uint8_t logical_channel, uint32_t &hal_channel)
{
    const uint8_t physical_channel = static_cast<uint8_t>(map.channel_base + logical_channel);
    switch (physical_channel) {
    case 1U:
        hal_channel = TIM_CHANNEL_1;
        return true;
    case 2U:
        hal_channel = TIM_CHANNEL_2;
        return true;
    case 3U:
        hal_channel = TIM_CHANNEL_3;
        return true;
    case 4U:
        hal_channel = TIM_CHANNEL_4;
        return true;
    default:
        return false;
    }
}

static bool logical_channel_to_index(const pwm_map_t &map, uint8_t logical_channel, std::uint8_t &index)
{
    const uint8_t physical_channel = static_cast<uint8_t>(map.channel_base + logical_channel);
    if ((physical_channel < 1U) || (physical_channel > k_tim_channel_count)) {
        return false;
    }
    index = static_cast<std::uint8_t>(physical_channel - 1U);
    return true;
}

static uint32_t timer_source_clock_hz(TIM_TypeDef *timer)
{
    RCC_ClkInitTypeDef clk = {};
    uint32_t latency = 0U;
    HAL_RCC_GetClockConfig(&clk, &latency);

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
        return (clk.APB2CLKDivider == RCC_HCLK_DIV1) ? pclk : (pclk * 2U);
    }

    const uint32_t pclk = HAL_RCC_GetPCLK1Freq();
    return (clk.APB1CLKDivider == RCC_HCLK_DIV1) ? pclk : (pclk * 2U);
}

static int32_t init_tim1_resource(pwm_map_t &map)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_TIM1_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {};
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF1_TIM1;

    gpio.Pin = GPIO_PIN_8;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = GPIO_PIN_13;
    HAL_GPIO_Init(GPIOB, &gpio);

    map.handle->Instance = map.base;
    map.handle->Init.Prescaler = 0U;
    map.handle->Init.CounterMode = TIM_COUNTERMODE_UP;
    map.handle->Init.Period = 999U;
    map.handle->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    map.handle->Init.RepetitionCounter = 0U;
    map.handle->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(map.handle) != HAL_OK) {
        return BSP_ERROR;
    }
    if (HAL_TIM_PWM_Init(map.handle) != HAL_OK) {
        return BSP_ERROR;
    }

    TIM_BreakDeadTimeConfigTypeDef bdtr = {};
    bdtr.OffStateRunMode = TIM_OSSR_DISABLE;
    bdtr.OffStateIDLEMode = TIM_OSSI_DISABLE;
    bdtr.LockLevel = TIM_LOCKLEVEL_OFF;
    bdtr.DeadTime = 0U;
    bdtr.BreakState = TIM_BREAK_DISABLE;
    bdtr.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
    bdtr.BreakFilter = 0U;
    bdtr.Break2State = TIM_BREAK2_DISABLE;
    bdtr.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
    bdtr.Break2Filter = 0U;
    bdtr.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
    if (HAL_TIMEx_ConfigBreakDeadTime(map.handle, &bdtr) != HAL_OK) {
        return BSP_ERROR;
    }

    HAL_NVIC_SetPriority(TIM1_UP_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(TIM1_UP_IRQn);
    HAL_NVIC_SetPriority(TIM1_CC_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(TIM1_CC_IRQn);
    return BSP_OK;
}

static int32_t ensure_initialized(bsp_pwm_instance_t instance)
{
    if (instance >= BSP_PWM_MAX) {
        return BSP_ERROR_PARAM;
    }

    attach_pwm_bindings();
    auto &map = s_pwm_map[instance];
    if (!map.has_pinmux) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (map.backend == BSP_PWM_BACKEND_STM32_HRTIM) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (map.backend != BSP_PWM_BACKEND_STM32_TIM) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (map.handle == nullptr) {
        return BSP_ERROR_PARAM;
    }
    if (map.initialized) {
        return BSP_OK;
    }

    if (instance == BSP_PWM_MAIN) {
        const auto status = init_tim1_resource(map);
        if (status != BSP_OK) {
            return status;
        }
    } else if (map.init_fn != nullptr) {
        map.init_fn();
    } else {
        return BSP_ERROR_PARAM;
    }

    map.initialized = true;
    return BSP_OK;
}

static int32_t configure_pwm_channel(pwm_map_t &map, uint32_t hal_channel, uint32_t pulse)
{
    TIM_OC_InitTypeDef oc = {};
    oc.OCMode = TIM_OCMODE_PWM1;
    oc.Pulse = pulse;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    oc.OCFastMode = TIM_OCFAST_DISABLE;
    oc.OCIdleState = TIM_OCIDLESTATE_RESET;
    oc.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    return (HAL_TIM_PWM_ConfigChannel(map.handle, &oc, hal_channel) == HAL_OK) ? BSP_OK : BSP_ERROR;
}

static int32_t apply_duty(pwm_map_t &map, uint8_t logical_channel, float duty_percent)
{
    uint32_t hal_channel = 0U;
    if (!logical_channel_to_hal(map, logical_channel, hal_channel)) {
        return BSP_ERROR_PARAM;
    }

    if (duty_percent < 0.0f) {
        duty_percent = 0.0f;
    }
    if (duty_percent > 100.0f) {
        duty_percent = 100.0f;
    }

    const auto period = __HAL_TIM_GET_AUTORELOAD(map.handle) + 1U;
    auto pulse = static_cast<uint32_t>((duty_percent / 100.0f) * static_cast<float>(period));
    if (pulse >= period) {
        pulse = period - 1U;
    }
    const auto config_status = configure_pwm_channel(map, hal_channel, pulse);
    if (config_status != BSP_OK) {
        return config_status;
    }

    __HAL_TIM_SET_COMPARE(map.handle, hal_channel, pulse);
    return BSP_OK;
}

static int32_t configure_trigger_compare(pwm_map_t &map, uint32_t compare_ticks)
{
    if (!pwm_supports(map, BSP_PWM_FEATURE_TRIGGER_COMPARE)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    const auto arr = __HAL_TIM_GET_AUTORELOAD(map.handle);
    if (compare_ticks > arr) {
        return BSP_ERROR_PARAM;
    }

    TIM_OC_InitTypeDef oc = {};
    oc.OCMode = TIM_OCMODE_PWM1;
    oc.Pulse = compare_ticks;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    oc.OCFastMode = TIM_OCFAST_DISABLE;
    oc.OCIdleState = TIM_OCIDLESTATE_RESET;
    oc.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    if (HAL_TIM_PWM_ConfigChannel(map.handle, &oc, TIM_CHANNEL_4) != HAL_OK) {
        return BSP_ERROR;
    }

    map.trigger_compare_configured = true;
    map.trigger_compare_ticks = compare_ticks;
    __HAL_TIM_SET_COMPARE(map.handle, TIM_CHANNEL_4, compare_ticks);
    return BSP_OK;
}

} // namespace

int32_t bsp_pwm_init(bsp_pwm_instance_t instance, uint32_t freq_hz, bsp_pwm_align_t align_mode)
{
    if ((instance >= BSP_PWM_MAX) || (freq_hz == 0U)) {
        return BSP_ERROR_PARAM;
    }

    attach_pwm_bindings();
    const auto &selected_map = s_pwm_map[instance];
    if ((align_mode == BSP_PWM_ALIGN_EDGE) && !pwm_supports(selected_map, BSP_PWM_FEATURE_EDGE_ALIGNED)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if ((align_mode == BSP_PWM_ALIGN_CENTER) && !pwm_supports(selected_map, BSP_PWM_FEATURE_CENTER_ALIGNED)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    const auto init_status = ensure_initialized(instance);
    if (init_status != BSP_OK) {
        return init_status;
    }

    auto &map = s_pwm_map[instance];
    const uint32_t clock_hz = timer_source_clock_hz(map.base);
    if (clock_hz == 0U) {
        return BSP_ERROR_UNSUPPORTED;
    }

    const uint64_t max_ticks = 65536ULL;
    const uint64_t divider_den = static_cast<uint64_t>(freq_hz) * max_ticks;
    uint32_t divider = static_cast<uint32_t>((static_cast<uint64_t>(clock_hz) + divider_den - 1ULL) / divider_den);
    if (divider == 0U) {
        divider = 1U;
    }
    if (divider > 65536U) {
        return BSP_ERROR_PARAM;
    }

    const uint32_t ticks = static_cast<uint32_t>(static_cast<uint64_t>(clock_hz) /
                                                (static_cast<uint64_t>(divider) * freq_hz));
    if ((ticks <= 1U) || (ticks > 65536U)) {
        return BSP_ERROR_PARAM;
    }

    (void) HAL_TIM_Base_Stop(map.handle);

    map.handle->Init.Prescaler = divider - 1U;
    map.handle->Init.CounterMode = (align_mode == BSP_PWM_ALIGN_CENTER) ? TIM_COUNTERMODE_CENTERALIGNED1 : TIM_COUNTERMODE_UP;
    map.handle->Init.Period = ticks - 1U;
    map.handle->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    map.handle->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (pwm_supports(map, BSP_PWM_FEATURE_COMPLEMENTARY)) {
        map.handle->Init.RepetitionCounter = 0U;
    }
    if (HAL_TIM_PWM_Init(map.handle) != HAL_OK) {
        return BSP_ERROR;
    }

    const auto duty_status = apply_duty(map, 0U, 0.0f);
    if (duty_status != BSP_OK) {
        return duty_status;
    }

    map.frequency_hz = clock_hz / (divider * ticks);
    map.align = align_mode;
    for (auto &enabled : map.output_enabled) {
        enabled = false;
    }
    for (auto &enabled : map.pair_enabled) {
        enabled = false;
    }
    map.running = false;
    if (map.trigger_compare_configured) {
        const auto arr = __HAL_TIM_GET_AUTORELOAD(map.handle);
        const auto compare_ticks = (map.trigger_compare_ticks > arr) ? (arr / 2U) : map.trigger_compare_ticks;
        const auto trigger_status = configure_trigger_compare(map, compare_ticks);
        if (trigger_status != BSP_OK) {
            return trigger_status;
        }
    }
    return BSP_OK;
}

int32_t bsp_pwm_get_frequency(bsp_pwm_instance_t instance, uint32_t *freq_hz)
{
    if ((instance >= BSP_PWM_MAX) || (freq_hz == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    const auto &map = s_pwm_map[instance];
    if (!map.initialized || (map.frequency_hz == 0U)) {
        return BSP_ERROR;
    }

    *freq_hz = map.frequency_hz;
    return BSP_OK;
}

int32_t bsp_pwm_set_duty(bsp_pwm_instance_t instance, uint8_t channel, float duty_percent)
{
    if (instance >= BSP_PWM_MAX) {
        return BSP_ERROR_PARAM;
    }
    const auto init_status = ensure_initialized(instance);
    if (init_status != BSP_OK) {
        return init_status;
    }
    return apply_duty(s_pwm_map[instance], channel, duty_percent);
}

int32_t bsp_pwm_set_compare_ticks(bsp_pwm_instance_t instance, uint8_t compare_index, uint32_t compare_ticks)
{
    if (instance >= BSP_PWM_MAX) {
        return BSP_ERROR_PARAM;
    }
    const auto init_status = ensure_initialized(instance);
    if (init_status != BSP_OK) {
        return init_status;
    }

    auto &map = s_pwm_map[instance];
    const auto arr = __HAL_TIM_GET_AUTORELOAD(map.handle);
    if (compare_ticks > arr) {
        return BSP_ERROR_PARAM;
    }

    if (compare_index == 0U) {
        return apply_duty(map, 0U, (static_cast<float>(compare_ticks) * 100.0f) / static_cast<float>(arr + 1U));
    }
    if (compare_index == 4U) {
        const auto status = configure_trigger_compare(map, compare_ticks);
        if ((status == BSP_OK) && map.running) {
            return (HAL_TIM_PWM_Start(map.handle, TIM_CHANNEL_4) == HAL_OK) ? BSP_OK : BSP_ERROR_BUSY;
        }
        return status;
    }

    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_pwm_enable_output(bsp_pwm_instance_t instance, uint8_t channel, bsp_state_t state)
{
    if (instance >= BSP_PWM_MAX) {
        return BSP_ERROR_PARAM;
    }

    attach_pwm_bindings();
    auto &map = s_pwm_map[instance];
    std::uint8_t index = 0U;
    if (!logical_channel_to_index(map, channel, index)) {
        return BSP_ERROR_PARAM;
    }

    map.output_enabled[index] = (state == BSP_ENABLE);
    return BSP_OK;
}

int32_t bsp_pwm_enable_pair(bsp_pwm_instance_t instance, uint8_t channel, bsp_state_t state)
{
    if (instance >= BSP_PWM_MAX) {
        return BSP_ERROR_PARAM;
    }

    attach_pwm_bindings();
    auto &map = s_pwm_map[instance];
    std::uint8_t index = 0U;
    if (!logical_channel_to_index(map, channel, index)) {
        return BSP_ERROR_PARAM;
    }
    if (!pwm_supports(map, BSP_PWM_FEATURE_COMPLEMENTARY)) {
        return (state == BSP_DISABLE) ? BSP_OK : BSP_ERROR_UNSUPPORTED;
    }
    map.pair_enabled[index] = (state == BSP_ENABLE);
    return BSP_OK;
}

int32_t bsp_pwm_set_deadtime(bsp_pwm_instance_t instance, uint8_t channel, uint16_t deadtime_ticks)
{
    if (instance >= BSP_PWM_MAX) {
        return BSP_ERROR_PARAM;
    }
    const auto init_status = ensure_initialized(instance);
    if (init_status != BSP_OK) {
        return init_status;
    }

    auto &map = s_pwm_map[instance];
    std::uint8_t index = 0U;
    if (!logical_channel_to_index(map, channel, index)) {
        return BSP_ERROR_PARAM;
    }
    if (!pwm_supports(map, BSP_PWM_FEATURE_DEADTIME)) {
        return (deadtime_ticks == 0U) ? BSP_OK : BSP_ERROR_UNSUPPORTED;
    }

    TIM_BreakDeadTimeConfigTypeDef bdtr = {};
    bdtr.OffStateRunMode = TIM_OSSR_DISABLE;
    bdtr.OffStateIDLEMode = TIM_OSSI_DISABLE;
    bdtr.LockLevel = TIM_LOCKLEVEL_OFF;
    bdtr.DeadTime = static_cast<uint8_t>(deadtime_ticks);
    bdtr.BreakState = TIM_BREAK_DISABLE;
    bdtr.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
    bdtr.BreakFilter = 0U;
    bdtr.Break2State = TIM_BREAK2_DISABLE;
    bdtr.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
    bdtr.Break2Filter = 0U;
    bdtr.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
    return (HAL_TIMEx_ConfigBreakDeadTime(map.handle, &bdtr) == HAL_OK) ? BSP_OK : BSP_ERROR;
}

int32_t bsp_pwm_force_output(bsp_pwm_instance_t instance, uint8_t channel, bsp_level_t level)
{
    if (instance >= BSP_PWM_MAX) {
        return BSP_ERROR_PARAM;
    }
    const auto init_status = ensure_initialized(instance);
    if (init_status != BSP_OK) {
        return init_status;
    }
    return apply_duty(s_pwm_map[instance], channel, (level == BSP_LEVEL_HIGH) ? 100.0f : 0.0f);
}

int32_t bsp_pwm_start(bsp_pwm_instance_t instance)
{
    if (instance >= BSP_PWM_MAX) {
        return BSP_ERROR_PARAM;
    }
    const auto init_status = ensure_initialized(instance);
    if (init_status != BSP_OK) {
        return init_status;
    }

    auto &map = s_pwm_map[instance];
    const auto base_status = (map.callback != nullptr) ? HAL_TIM_Base_Start_IT(map.handle) : HAL_TIM_Base_Start(map.handle);
    if (base_status != HAL_OK) {
        return BSP_ERROR_BUSY;
    }

    for (std::uint8_t index = 0U; index < k_tim_channel_count; ++index) {
        const uint32_t hal_channel = (index == 0U) ? TIM_CHANNEL_1 :
                                     (index == 1U) ? TIM_CHANNEL_2 :
                                     (index == 2U) ? TIM_CHANNEL_3 :
                                                     TIM_CHANNEL_4;
        if (map.output_enabled[index] && HAL_TIM_PWM_Start(map.handle, hal_channel) != HAL_OK) {
            return BSP_ERROR_BUSY;
        }
        if (map.pair_enabled[index] && HAL_TIMEx_PWMN_Start(map.handle, hal_channel) != HAL_OK) {
            return BSP_ERROR_BUSY;
        }
    }

    if (map.trigger_compare_configured && HAL_TIM_PWM_Start(map.handle, TIM_CHANNEL_4) != HAL_OK) {
        return BSP_ERROR_BUSY;
    }
    if (map.callback != nullptr) {
        __HAL_TIM_ENABLE_IT(map.handle, TIM_IT_CC1);
    }
    map.running = true;
    return BSP_OK;
}

int32_t bsp_pwm_stop(bsp_pwm_instance_t instance)
{
    if (instance >= BSP_PWM_MAX) {
        return BSP_ERROR_PARAM;
    }

    auto &map = s_pwm_map[instance];
    if (map.handle == nullptr) {
        return BSP_ERROR;
    }

    for (std::uint8_t index = 0U; index < k_tim_channel_count; ++index) {
        const uint32_t hal_channel = (index == 0U) ? TIM_CHANNEL_1 :
                                     (index == 1U) ? TIM_CHANNEL_2 :
                                     (index == 2U) ? TIM_CHANNEL_3 :
                                                     TIM_CHANNEL_4;
        (void) HAL_TIM_PWM_Stop(map.handle, hal_channel);
        if (pwm_supports(map, BSP_PWM_FEATURE_COMPLEMENTARY)) {
            (void) HAL_TIMEx_PWMN_Stop(map.handle, hal_channel);
        }
    }
    if (pwm_supports(map, BSP_PWM_FEATURE_TRIGGER_COMPARE)) {
        (void) HAL_TIM_PWM_Stop(map.handle, TIM_CHANNEL_4);
    }
    (void) HAL_TIM_Base_Stop_IT(map.handle);
    (void) HAL_TIM_Base_Stop(map.handle);
    __HAL_TIM_DISABLE_IT(map.handle, TIM_IT_CC1);
    map.running = false;
    return BSP_OK;
}

int32_t bsp_pwm_register_event_callback(bsp_pwm_instance_t instance, bsp_pwm_event_callback_t callback, void *arg)
{
    if (instance >= BSP_PWM_MAX) {
        return BSP_ERROR_PARAM;
    }
    s_pwm_map[instance].callback = callback;
    s_pwm_map[instance].callback_arg = arg;
    return BSP_OK;
}

extern "C" void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim)
{
    for (std::uint32_t index = 0; index < BSP_PWM_MAX; ++index) {
        auto &map = s_pwm_map[index];
        if ((map.handle == htim) && (map.callback != nullptr)) {
            map.callback(BSP_PWM_EVENT_COMPARE, map.callback_arg);
            break;
        }
    }
}

extern "C" void bsp_stm32_pwm_period_callback(TIM_HandleTypeDef *htim)
{
    for (std::uint32_t index = 0; index < BSP_PWM_MAX; ++index) {
        auto &map = s_pwm_map[index];
        if ((map.handle == htim) && (map.callback != nullptr)) {
            map.callback(BSP_PWM_EVENT_RELOAD, map.callback_arg);
            break;
        }
    }
}

extern "C" void bsp_stm32_tim1_up_irq_handler(void)
{
    attach_pwm_bindings();
    if (s_pwm_map[BSP_PWM_MAIN].initialized) {
        HAL_TIM_IRQHandler(s_pwm_map[BSP_PWM_MAIN].handle);
    }
}

extern "C" void bsp_stm32_tim1_cc_irq_handler(void)
{
    attach_pwm_bindings();
    if (s_pwm_map[BSP_PWM_MAIN].initialized) {
        HAL_TIM_IRQHandler(s_pwm_map[BSP_PWM_MAIN].handle);
    }
}

extern "C" __attribute__((weak)) void TIM1_UP_IRQHandler(void)
{
    bsp_stm32_tim1_up_irq_handler();
}

extern "C" __attribute__((weak)) void TIM1_CC_IRQHandler(void)
{
    bsp_stm32_tim1_cc_irq_handler();
}

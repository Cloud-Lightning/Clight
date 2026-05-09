#include "bsp_i2c.h"

#include <cstdint>
#include <cstdlib>

extern "C" {
#include "main.h"
}

namespace {

typedef struct {
    I2C_TypeDef *base;
    IRQn_Type irq;
    bool has_pinmux;
    std::uint32_t features;
    I2C_HandleTypeDef handle;
    bsp_transfer_mode_t transfer_mode;
    bool initialized;
    uint32_t timing;
    bool ten_bit_addressing;
    bsp_i2c_event_callback_t event_callback;
    void *event_callback_arg;
    bsp_i2c_event_t pending_event;
} i2c_map_t;

#define BSP_I2C_MAP_ITEM(name, base, irq, clk, has_pinmux, features) \
    { base, irq, (has_pinmux) != 0U, static_cast<std::uint32_t>(features), {}, BSP_TRANSFER_MODE_AUTO, false, 0U, false, nullptr, nullptr, BSP_I2C_EVENT_WRITE },
static i2c_map_t s_i2c_map[BSP_I2C_MAX] = {
    BSP_I2C_LIST(BSP_I2C_MAP_ITEM)
};
#undef BSP_I2C_MAP_ITEM

typedef struct {
    uint32_t max_frequency_hz;
    uint32_t low_min_ns;
    uint32_t high_min_ns;
    uint32_t setup_min_ns;
    uint32_t rise_ns;
    uint32_t fall_ns;
} i2c_timing_spec_t;

static bool i2c_has_feature(const i2c_map_t &map, std::uint32_t feature)
{
    return (map.features & feature) != 0U;
}

static int32_t validate_i2c_init_mode(const i2c_map_t &map, bsp_transfer_mode_t transfer_mode)
{
    switch (transfer_mode) {
    case BSP_TRANSFER_MODE_AUTO:
        return BSP_OK;
    case BSP_TRANSFER_MODE_BLOCKING:
        return i2c_has_feature(map, BSP_I2C_FEATURE_BLOCKING) ? BSP_OK : BSP_ERROR_UNSUPPORTED;
    case BSP_TRANSFER_MODE_INTERRUPT:
        return i2c_has_feature(map, BSP_I2C_FEATURE_INTERRUPT) ? BSP_OK : BSP_ERROR_UNSUPPORTED;
    case BSP_TRANSFER_MODE_DMA:
        return i2c_has_feature(map, BSP_I2C_FEATURE_DMA) ? BSP_OK : BSP_ERROR_UNSUPPORTED;
    default:
        return BSP_ERROR_PARAM;
    }
}

static bool i2c_dma_available(const i2c_map_t &map, bool write)
{
    if (!i2c_has_feature(map, BSP_I2C_FEATURE_DMA)) {
        return false;
    }
    return write ? (map.handle.hdmatx != nullptr) : (map.handle.hdmarx != nullptr);
}

static int32_t resolve_i2c_async_mode(const i2c_map_t &map, bool write, bsp_transfer_mode_t *out_mode)
{
    switch (map.transfer_mode) {
    case BSP_TRANSFER_MODE_AUTO:
        if (i2c_dma_available(map, write)) {
            *out_mode = BSP_TRANSFER_MODE_DMA;
            return BSP_OK;
        }
        if (i2c_has_feature(map, BSP_I2C_FEATURE_INTERRUPT)) {
            *out_mode = BSP_TRANSFER_MODE_INTERRUPT;
            return BSP_OK;
        }
        return BSP_ERROR_UNSUPPORTED;
    case BSP_TRANSFER_MODE_DMA:
        if (i2c_dma_available(map, write)) {
            *out_mode = BSP_TRANSFER_MODE_DMA;
            return BSP_OK;
        }
        return BSP_ERROR_UNSUPPORTED;
    case BSP_TRANSFER_MODE_INTERRUPT:
        if (i2c_has_feature(map, BSP_I2C_FEATURE_INTERRUPT)) {
            *out_mode = BSP_TRANSFER_MODE_INTERRUPT;
            return BSP_OK;
        }
        return BSP_ERROR_UNSUPPORTED;
    default:
        return BSP_ERROR_UNSUPPORTED;
    }
}

static uint32_t i2c_kernel_clock_hz()
{
#if defined(RCC_PERIPHCLK_I2C123)
    auto clock = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_I2C123);
    if (clock != 0U) {
        return clock;
    }
#endif
    return HAL_RCC_GetPCLK1Freq();
}

static uint32_t div_ceil_u32(uint32_t numerator, uint32_t denominator)
{
    return (numerator + denominator - 1U) / denominator;
}

static bool i2c_select_spec(uint32_t frequency_hz, i2c_timing_spec_t *spec)
{
    if ((frequency_hz == 0U) || (spec == nullptr)) {
        return false;
    }
    if (frequency_hz <= 100000U) {
        *spec = {100000U, 4700U, 4000U, 250U, 1000U, 300U};
        return true;
    }
    if (frequency_hz <= 400000U) {
        *spec = {400000U, 1300U, 600U, 100U, 300U, 300U};
        return true;
    }
    if (frequency_hz <= 1000000U) {
        *spec = {1000000U, 500U, 260U, 50U, 120U, 120U};
        return true;
    }
    return false;
}

static bool calculate_i2c_timing(uint32_t frequency_hz, uint32_t *timing)
{
    if (timing == nullptr) {
        return false;
    }

    i2c_timing_spec_t spec{};
    if (!i2c_select_spec(frequency_hz, &spec)) {
        return false;
    }

    const uint32_t clock_hz = i2c_kernel_clock_hz();
    if (clock_hz == 0U) {
        return false;
    }

    const uint64_t target_period_ns = 1000000000ULL / frequency_hz;
    bool found = false;
    uint64_t best_error = UINT64_MAX;
    uint32_t best_timing = 0U;

    for (uint32_t presc = 0U; presc <= 15U; ++presc) {
        const uint64_t t_presc_ns = (1000000000ULL * static_cast<uint64_t>(presc + 1U)) / clock_hz;
        if (t_presc_ns == 0ULL) {
            continue;
        }

        const uint32_t scldel = div_ceil_u32(spec.setup_min_ns, static_cast<uint32_t>(t_presc_ns));
        if ((scldel == 0U) || (scldel > 16U)) {
            continue;
        }
        const uint32_t scll_min = div_ceil_u32(spec.low_min_ns, static_cast<uint32_t>(t_presc_ns));
        const uint32_t sclh_min = div_ceil_u32(spec.high_min_ns, static_cast<uint32_t>(t_presc_ns));
        if ((scll_min == 0U) || (sclh_min == 0U) || (scll_min > 256U) || (sclh_min > 256U)) {
            continue;
        }

        for (uint32_t scll = scll_min - 1U; scll < 256U; ++scll) {
            for (uint32_t sclh = sclh_min - 1U; sclh < 256U; ++sclh) {
                const uint64_t period_ns =
                    (static_cast<uint64_t>(scll + sclh + 2U) * t_presc_ns) + spec.rise_ns + spec.fall_ns;
                if (period_ns < target_period_ns) {
                    continue;
                }
                const uint64_t error = period_ns - target_period_ns;
                if (!found || (error < best_error)) {
                    found = true;
                    best_error = error;
                    best_timing = ((presc & 0xFU) << 28U) |
                                  (((scldel - 1U) & 0xFU) << 20U) |
                                  (0U << 16U) |
                                  ((sclh & 0xFFU) << 8U) |
                                  (scll & 0xFFU);
                    if (error == 0ULL) {
                        *timing = best_timing;
                        return true;
                    }
                }
            }
        }
    }

    if (!found) {
        return false;
    }
    *timing = best_timing;
    return true;
}

} // namespace

int32_t bsp_i2c_init(bsp_i2c_instance_t i2c,
                     uint32_t frequency_hz,
                     bsp_state_t ten_bit_addressing,
                     bsp_transfer_mode_t transfer_mode,
                     uint8_t irq_priority)
{
    if (i2c >= BSP_I2C_MAX) {
        return BSP_ERROR_PARAM;
    }

    auto &map = s_i2c_map[i2c];
    if (!map.has_pinmux) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (!i2c_has_feature(map, BSP_I2C_FEATURE_MASTER)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if ((frequency_hz != 0U) && !i2c_has_feature(map, BSP_I2C_FEATURE_CONFIGURABLE_CLOCK)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if ((ten_bit_addressing == BSP_ENABLE) && !i2c_has_feature(map, BSP_I2C_FEATURE_TEN_BIT_ADDRESS)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    const auto mode_status = validate_i2c_init_mode(map, transfer_mode);
    if (mode_status != BSP_OK) {
        return mode_status;
    }

    uint32_t timing = BSP_I2C_MAIN_TIMING;
    if (frequency_hz != 0U) {
        if (!calculate_i2c_timing(frequency_hz, &timing)) {
            return BSP_ERROR_UNSUPPORTED;
        }
    }

    const bool use_ten_bit = (ten_bit_addressing == BSP_ENABLE);
    if (map.initialized && (map.timing == timing) && (map.ten_bit_addressing == use_ten_bit)) {
        map.transfer_mode = transfer_mode;
        return BSP_OK;
    }
    if (map.initialized) {
        (void) HAL_I2C_DeInit(&map.handle);
        map.initialized = false;
    }

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {};
    gpio.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    gpio.Mode = GPIO_MODE_AF_OD;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &gpio);

    map.handle.Instance = map.base;
    map.handle.Init.Timing = timing;
    map.handle.Init.OwnAddress1 = 0U;
    map.handle.Init.AddressingMode = use_ten_bit ? I2C_ADDRESSINGMODE_10BIT : I2C_ADDRESSINGMODE_7BIT;
    map.handle.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    map.handle.Init.OwnAddress2 = 0U;
    map.handle.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    map.handle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    map.handle.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&map.handle) != HAL_OK) {
        return BSP_ERROR;
    }
    if (HAL_I2CEx_ConfigAnalogFilter(&map.handle, I2C_ANALOGFILTER_ENABLE) != HAL_OK) {
        return BSP_ERROR;
    }
    if (HAL_I2CEx_ConfigDigitalFilter(&map.handle, 0U) != HAL_OK) {
        return BSP_ERROR;
    }

    HAL_NVIC_SetPriority(I2C1_EV_IRQn, (irq_priority == 0U) ? BSP_IRQ_PRIORITY_DEFAULT : irq_priority, 0);
    HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
    HAL_NVIC_SetPriority(I2C1_ER_IRQn, (irq_priority == 0U) ? BSP_IRQ_PRIORITY_DEFAULT : irq_priority, 0);
    HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);

    map.transfer_mode = transfer_mode;
    map.timing = timing;
    map.ten_bit_addressing = use_ten_bit;
    map.initialized = true;
    return BSP_OK;
}

int32_t bsp_i2c_write(bsp_i2c_instance_t i2c, uint16_t device_address, const uint8_t *data, uint32_t size)
{
    if ((i2c >= BSP_I2C_MAX) || (data == nullptr) || (size == 0U)) {
        return BSP_ERROR_PARAM;
    }
    const auto init_status = bsp_i2c_init(i2c, 0U, BSP_DISABLE, BSP_TRANSFER_MODE_AUTO, 0U);
    if (init_status != BSP_OK) {
        return init_status;
    }
    if (!i2c_has_feature(s_i2c_map[i2c], BSP_I2C_FEATURE_BLOCKING)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    return (HAL_I2C_Master_Transmit(&s_i2c_map[i2c].handle, static_cast<uint16_t>(device_address << 1U), const_cast<uint8_t *>(data), static_cast<uint16_t>(size), HAL_MAX_DELAY) == HAL_OK)
               ? BSP_OK
               : BSP_ERROR_TIMEOUT;
}

int32_t bsp_i2c_read(bsp_i2c_instance_t i2c, uint16_t device_address, uint8_t *data, uint32_t size)
{
    if ((i2c >= BSP_I2C_MAX) || (data == nullptr) || (size == 0U)) {
        return BSP_ERROR_PARAM;
    }
    const auto init_status = bsp_i2c_init(i2c, 0U, BSP_DISABLE, BSP_TRANSFER_MODE_AUTO, 0U);
    if (init_status != BSP_OK) {
        return init_status;
    }
    if (!i2c_has_feature(s_i2c_map[i2c], BSP_I2C_FEATURE_BLOCKING)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    return (HAL_I2C_Master_Receive(&s_i2c_map[i2c].handle, static_cast<uint16_t>(device_address << 1U), data, static_cast<uint16_t>(size), HAL_MAX_DELAY) == HAL_OK)
               ? BSP_OK
               : BSP_ERROR_TIMEOUT;
}

int32_t bsp_i2c_write_async(bsp_i2c_instance_t i2c, uint16_t device_address, const uint8_t *data, uint32_t size)
{
    if ((i2c >= BSP_I2C_MAX) || (data == nullptr) || (size == 0U)) {
        return BSP_ERROR_PARAM;
    }
    const auto init_status = bsp_i2c_init(i2c, 0U, BSP_DISABLE, BSP_TRANSFER_MODE_AUTO, 0U);
    if (init_status != BSP_OK) {
        return init_status;
    }
    if (s_i2c_map[i2c].event_callback == nullptr) {
        return BSP_ERROR_PARAM;
    }
    auto &map = s_i2c_map[i2c];
    bsp_transfer_mode_t async_mode = BSP_TRANSFER_MODE_AUTO;
    const auto mode_status = resolve_i2c_async_mode(map, true, &async_mode);
    if (mode_status != BSP_OK) {
        return mode_status;
    }
    map.pending_event = BSP_I2C_EVENT_WRITE;

    const auto status = (async_mode == BSP_TRANSFER_MODE_DMA)
                            ? HAL_I2C_Master_Transmit_DMA(
                                  &map.handle,
                                  static_cast<uint16_t>(device_address << 1U),
                                  const_cast<uint8_t *>(data),
                                  static_cast<uint16_t>(size))
                            : HAL_I2C_Master_Transmit_IT(
                                  &map.handle,
                                  static_cast<uint16_t>(device_address << 1U),
                                  const_cast<uint8_t *>(data),
                                  static_cast<uint16_t>(size));
    return (status == HAL_OK) ? BSP_OK : ((status == HAL_BUSY) ? BSP_ERROR_BUSY : BSP_ERROR);
}

int32_t bsp_i2c_read_async(bsp_i2c_instance_t i2c, uint16_t device_address, uint8_t *data, uint32_t size)
{
    if ((i2c >= BSP_I2C_MAX) || (data == nullptr) || (size == 0U)) {
        return BSP_ERROR_PARAM;
    }
    const auto init_status = bsp_i2c_init(i2c, 0U, BSP_DISABLE, BSP_TRANSFER_MODE_AUTO, 0U);
    if (init_status != BSP_OK) {
        return init_status;
    }
    if (s_i2c_map[i2c].event_callback == nullptr) {
        return BSP_ERROR_PARAM;
    }
    auto &map = s_i2c_map[i2c];
    bsp_transfer_mode_t async_mode = BSP_TRANSFER_MODE_AUTO;
    const auto mode_status = resolve_i2c_async_mode(map, false, &async_mode);
    if (mode_status != BSP_OK) {
        return mode_status;
    }
    map.pending_event = BSP_I2C_EVENT_READ;

    const auto status = (async_mode == BSP_TRANSFER_MODE_DMA)
                            ? HAL_I2C_Master_Receive_DMA(
                                  &map.handle,
                                  static_cast<uint16_t>(device_address << 1U),
                                  data,
                                  static_cast<uint16_t>(size))
                            : HAL_I2C_Master_Receive_IT(
                                  &map.handle,
                                  static_cast<uint16_t>(device_address << 1U),
                                  data,
                                  static_cast<uint16_t>(size));
    return (status == HAL_OK) ? BSP_OK : ((status == HAL_BUSY) ? BSP_ERROR_BUSY : BSP_ERROR);
}

int32_t bsp_i2c_register_event_callback(bsp_i2c_instance_t i2c, bsp_i2c_event_callback_t callback, void *arg)
{
    if (i2c >= BSP_I2C_MAX) {
        return BSP_ERROR_PARAM;
    }
    s_i2c_map[i2c].event_callback = callback;
    s_i2c_map[i2c].event_callback_arg = arg;
    return BSP_OK;
}

extern "C" void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    for (std::uint32_t index = 0; index < BSP_I2C_MAX; ++index) {
        auto &map = s_i2c_map[index];
        if ((&map.handle == hi2c) && (map.event_callback != nullptr)) {
            map.event_callback(map.pending_event, BSP_OK, map.event_callback_arg);
            break;
        }
    }
}

extern "C" void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    HAL_I2C_MasterTxCpltCallback(hi2c);
}

extern "C" void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    for (std::uint32_t index = 0; index < BSP_I2C_MAX; ++index) {
        auto &map = s_i2c_map[index];
        if ((&map.handle == hi2c) && (map.event_callback != nullptr)) {
            map.event_callback(map.pending_event, BSP_ERROR, map.event_callback_arg);
            break;
        }
    }
}

extern "C" void bsp_stm32_i2c1_ev_irq_handler(void)
{
    if (s_i2c_map[BSP_I2C_MAIN].initialized) {
        HAL_I2C_EV_IRQHandler(&s_i2c_map[BSP_I2C_MAIN].handle);
    }
}

extern "C" void bsp_stm32_i2c1_er_irq_handler(void)
{
    if (s_i2c_map[BSP_I2C_MAIN].initialized) {
        HAL_I2C_ER_IRQHandler(&s_i2c_map[BSP_I2C_MAIN].handle);
    }
}

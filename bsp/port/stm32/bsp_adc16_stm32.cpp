#include "bsp_adc16.h"

#include <cstdint>
#include <cstring>

extern "C" {
#include "main.h"
}

#if defined(HAL_ADC_MODULE_ENABLED)

namespace {

constexpr std::uint8_t kMaxSequenceChannels = 16U;
constexpr std::uint8_t kMaxInjectedChannels = 4U;

typedef struct {
    ADC_TypeDef *base;
    IRQn_Type irq;
    uint8_t default_channel;
    bool has_pinmux;
    uint32_t features;
    ADC_HandleTypeDef handle;
    bsp_adc16_init_config_t config;
    bool initialized;
    uint8_t active_channel;
    bool sequence_active;
    bool sequence_continuous;
    bool sequence_hardware_trigger;
    bool has_sequence_trigger_route;
    uint32_t sequence_external_trigger;
    uint32_t sequence_external_trigger_edge;
    uint8_t sequence_count;
    uint8_t sequence_index;
    uint8_t sequence_channels[kMaxSequenceChannels];
    bsp_adc16_sample_t sequence_samples[kMaxSequenceChannels];
    bool sequence_dma_active;
    bool sequence_dma_circular;
    uint32_t *sequence_dma_buffer;
    uint32_t sequence_dma_sample_count;
    bool preemption_active;
    bool preemption_hardware_trigger;
    bool has_preemption_trigger_route;
    uint32_t preemption_external_trigger;
    uint32_t preemption_external_trigger_edge;
    uint8_t preemption_trigger_channel;
    uint8_t preemption_count;
    uint8_t preemption_channels[kMaxInjectedChannels];
    bsp_adc16_sample_t preemption_samples[kMaxInjectedChannels];
    bsp_adc16_callback_t callback;
    void *callback_arg;
    bsp_adc16_sequence_callback_t sequence_callback;
    void *sequence_callback_arg;
    bsp_adc16_preemption_callback_t preemption_callback;
    void *preemption_callback_arg;
} adc16_map_t;

#define BSP_ADC16_MAP_ITEM(name, base, irq, clk, ch, clk_src_bus, has_pinmux, features) \
    { base, irq, static_cast<uint8_t>(ch), (has_pinmux) != 0U, static_cast<uint32_t>(features), {}, {}, false, static_cast<uint8_t>(ch), false, false, false, false, ADC_SOFTWARE_START, ADC_EXTERNALTRIGCONVEDGE_NONE, 0U, 0U, {}, {}, false, false, nullptr, 0U, false, false, false, ADC_INJECTED_SOFTWARE_START, ADC_EXTERNALTRIGINJECCONV_EDGE_NONE, 0U, 0U, {}, {}, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr },
static adc16_map_t s_adc16_map[BSP_ADC16_MAX] = {
    BSP_ADC16_LIST(BSP_ADC16_MAP_ITEM)
};
#undef BSP_ADC16_MAP_ITEM

static DMA_HandleTypeDef s_adc1_dma;
static bool s_adc1_dma_configured = false;

bool adc16_has_feature(const adc16_map_t &map, uint32_t feature)
{
    return (map.features & feature) != 0U;
}

uint32_t resolution_to_hal(bsp_adc16_resolution_t resolution)
{
    switch (resolution) {
    case BSP_ADC16_RESOLUTION_8_BITS: return ADC_RESOLUTION_8B;
    case BSP_ADC16_RESOLUTION_10_BITS: return ADC_RESOLUTION_10B;
    case BSP_ADC16_RESOLUTION_12_BITS: return ADC_RESOLUTION_12B;
    case BSP_ADC16_RESOLUTION_16_BITS:
    case BSP_ADC16_RESOLUTION_DEFAULT:
    default:
        return ADC_RESOLUTION_16B;
    }
}

uint32_t divider_to_hal(bsp_adc16_clock_divider_t divider)
{
    switch (divider) {
    case BSP_ADC16_CLOCK_DIVIDER_1: return ADC_CLOCK_ASYNC_DIV1;
    case BSP_ADC16_CLOCK_DIVIDER_2: return ADC_CLOCK_ASYNC_DIV2;
    case BSP_ADC16_CLOCK_DIVIDER_4: return ADC_CLOCK_ASYNC_DIV4;
    case BSP_ADC16_CLOCK_DIVIDER_6: return ADC_CLOCK_ASYNC_DIV6;
    case BSP_ADC16_CLOCK_DIVIDER_8: return ADC_CLOCK_ASYNC_DIV8;
    case BSP_ADC16_CLOCK_DIVIDER_10: return ADC_CLOCK_ASYNC_DIV10;
    case BSP_ADC16_CLOCK_DIVIDER_12: return ADC_CLOCK_ASYNC_DIV12;
    case BSP_ADC16_CLOCK_DIVIDER_16: return ADC_CLOCK_ASYNC_DIV16;
    case BSP_ADC16_CLOCK_DIVIDER_DEFAULT:
    default:
        return ADC_CLOCK_ASYNC_DIV4;
    }
}

bool is_supported_config(const bsp_adc16_init_config_t &config)
{
    switch (config.resolution) {
    case BSP_ADC16_RESOLUTION_DEFAULT:
    case BSP_ADC16_RESOLUTION_8_BITS:
    case BSP_ADC16_RESOLUTION_10_BITS:
    case BSP_ADC16_RESOLUTION_12_BITS:
    case BSP_ADC16_RESOLUTION_16_BITS:
        break;
    default:
        return false;
    }

    switch (config.clock_divider) {
    case BSP_ADC16_CLOCK_DIVIDER_DEFAULT:
    case BSP_ADC16_CLOCK_DIVIDER_1:
    case BSP_ADC16_CLOCK_DIVIDER_2:
    case BSP_ADC16_CLOCK_DIVIDER_4:
    case BSP_ADC16_CLOCK_DIVIDER_6:
    case BSP_ADC16_CLOCK_DIVIDER_8:
    case BSP_ADC16_CLOCK_DIVIDER_10:
    case BSP_ADC16_CLOCK_DIVIDER_12:
    case BSP_ADC16_CLOCK_DIVIDER_16:
        return true;
    default:
        return false;
    }
}

int32_t validate_adc16_config_features(const adc16_map_t &map, const bsp_adc16_init_config_t &config)
{
    if (!is_supported_config(config)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if ((config.resolution != BSP_ADC16_RESOLUTION_DEFAULT) &&
        !adc16_has_feature(map, BSP_ADC16_FEATURE_CONFIGURABLE_RESOLUTION)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if ((config.clock_divider != BSP_ADC16_CLOCK_DIVIDER_DEFAULT) &&
        !adc16_has_feature(map, BSP_ADC16_FEATURE_CONFIGURABLE_CLOCK_DIVIDER)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    return BSP_OK;
}

bool is_supported_external_trigger(uint32_t trigger)
{
    switch (trigger) {
    case ADC_SOFTWARE_START:
    case ADC_EXTERNALTRIG_T1_TRGO:
    case ADC_EXTERNALTRIG_T1_TRGO2:
    case ADC_EXTERNALTRIG_T2_TRGO:
    case ADC_EXTERNALTRIG_T3_TRGO:
    case ADC_EXTERNALTRIG_T4_TRGO:
    case ADC_EXTERNALTRIG_T6_TRGO:
    case ADC_EXTERNALTRIG_T8_TRGO:
    case ADC_EXTERNALTRIG_T8_TRGO2:
    case ADC_EXTERNALTRIG_T15_TRGO:
#ifdef ADC_EXTERNALTRIG_T23_TRGO
    case ADC_EXTERNALTRIG_T23_TRGO:
#endif
#ifdef ADC_EXTERNALTRIG_T24_TRGO
    case ADC_EXTERNALTRIG_T24_TRGO:
#endif
        return true;
    default:
        return false;
    }
}

bool is_supported_injected_external_trigger(uint32_t trigger)
{
    switch (trigger) {
    case ADC_INJECTED_SOFTWARE_START:
#ifdef ADC_EXTERNALTRIGINJEC_T1_TRGO
    case ADC_EXTERNALTRIGINJEC_T1_TRGO:
#endif
#ifdef ADC_EXTERNALTRIGINJEC_T1_TRGO2
    case ADC_EXTERNALTRIGINJEC_T1_TRGO2:
#endif
#ifdef ADC_EXTERNALTRIGINJEC_T1_CC4
    case ADC_EXTERNALTRIGINJEC_T1_CC4:
#endif
#ifdef ADC_EXTERNALTRIGINJEC_T2_TRGO
    case ADC_EXTERNALTRIGINJEC_T2_TRGO:
#endif
#ifdef ADC_EXTERNALTRIGINJEC_T2_CC1
    case ADC_EXTERNALTRIGINJEC_T2_CC1:
#endif
#ifdef ADC_EXTERNALTRIGINJEC_T3_TRGO
    case ADC_EXTERNALTRIGINJEC_T3_TRGO:
#endif
#ifdef ADC_EXTERNALTRIGINJEC_T3_CC1
    case ADC_EXTERNALTRIGINJEC_T3_CC1:
#endif
#ifdef ADC_EXTERNALTRIGINJEC_T3_CC3
    case ADC_EXTERNALTRIGINJEC_T3_CC3:
#endif
#ifdef ADC_EXTERNALTRIGINJEC_T3_CC4
    case ADC_EXTERNALTRIGINJEC_T3_CC4:
#endif
#ifdef ADC_EXTERNALTRIGINJEC_T4_TRGO
    case ADC_EXTERNALTRIGINJEC_T4_TRGO:
#endif
#ifdef ADC_EXTERNALTRIGINJEC_T6_TRGO
    case ADC_EXTERNALTRIGINJEC_T6_TRGO:
#endif
#ifdef ADC_EXTERNALTRIGINJEC_T8_TRGO
    case ADC_EXTERNALTRIGINJEC_T8_TRGO:
#endif
#ifdef ADC_EXTERNALTRIGINJEC_T8_TRGO2
    case ADC_EXTERNALTRIGINJEC_T8_TRGO2:
#endif
#ifdef ADC_EXTERNALTRIGINJEC_T8_CC4
    case ADC_EXTERNALTRIGINJEC_T8_CC4:
#endif
#ifdef ADC_EXTERNALTRIGINJEC_T15_TRGO
    case ADC_EXTERNALTRIGINJEC_T15_TRGO:
#endif
        return true;
    default:
        return false;
    }
}

uint32_t trigger_route_edge(const bsp_adc16_trigger_route_t &route)
{
    switch (route.output) {
    case ADC_EXTERNALTRIGCONVEDGE_RISING:
    case ADC_EXTERNALTRIGCONVEDGE_FALLING:
    case ADC_EXTERNALTRIGCONVEDGE_RISINGFALLING:
        return route.output;
    default:
        break;
    }

    if (route.type == BSP_ADC16_TRIGGER_OUTPUT_PULSE_BOTH) {
        return ADC_EXTERNALTRIGCONVEDGE_RISINGFALLING;
    }
    if (route.type == BSP_ADC16_TRIGGER_OUTPUT_PULSE_FALLING) {
        return (route.invert == BSP_ENABLE) ? ADC_EXTERNALTRIGCONVEDGE_RISING : ADC_EXTERNALTRIGCONVEDGE_FALLING;
    }
    return (route.invert == BSP_ENABLE) ? ADC_EXTERNALTRIGCONVEDGE_FALLING : ADC_EXTERNALTRIGCONVEDGE_RISING;
}

uint32_t injected_trigger_route_edge(const bsp_adc16_trigger_route_t &route)
{
    switch (route.output) {
    case ADC_EXTERNALTRIGINJECCONV_EDGE_RISING:
    case ADC_EXTERNALTRIGINJECCONV_EDGE_FALLING:
    case ADC_EXTERNALTRIGINJECCONV_EDGE_RISINGFALLING:
        return route.output;
    default:
        break;
    }

    if (route.type == BSP_ADC16_TRIGGER_OUTPUT_PULSE_BOTH) {
        return ADC_EXTERNALTRIGINJECCONV_EDGE_RISINGFALLING;
    }
    if (route.type == BSP_ADC16_TRIGGER_OUTPUT_PULSE_FALLING) {
        return (route.invert == BSP_ENABLE) ? ADC_EXTERNALTRIGINJECCONV_EDGE_RISING : ADC_EXTERNALTRIGINJECCONV_EDGE_FALLING;
    }
    return (route.invert == BSP_ENABLE) ? ADC_EXTERNALTRIGINJECCONV_EDGE_FALLING : ADC_EXTERNALTRIGINJECCONV_EDGE_RISING;
}

uint32_t rank_to_hal(uint8_t index)
{
    switch (index) {
    case 0U: return ADC_REGULAR_RANK_1;
    case 1U: return ADC_REGULAR_RANK_2;
    case 2U: return ADC_REGULAR_RANK_3;
    case 3U: return ADC_REGULAR_RANK_4;
    case 4U: return ADC_REGULAR_RANK_5;
    case 5U: return ADC_REGULAR_RANK_6;
    case 6U: return ADC_REGULAR_RANK_7;
    case 7U: return ADC_REGULAR_RANK_8;
    case 8U: return ADC_REGULAR_RANK_9;
    case 9U: return ADC_REGULAR_RANK_10;
    case 10U: return ADC_REGULAR_RANK_11;
    case 11U: return ADC_REGULAR_RANK_12;
    case 12U: return ADC_REGULAR_RANK_13;
    case 13U: return ADC_REGULAR_RANK_14;
    case 14U: return ADC_REGULAR_RANK_15;
    default: return ADC_REGULAR_RANK_16;
    }
}

uint32_t injected_rank_to_hal(uint8_t index)
{
    switch (index) {
    case 0U: return ADC_INJECTED_RANK_1;
    case 1U: return ADC_INJECTED_RANK_2;
    case 2U: return ADC_INJECTED_RANK_3;
    default: return ADC_INJECTED_RANK_4;
    }
}

uint32_t channel_to_hal(uint8_t channel)
{
    switch (channel) {
    case 0U: return ADC_CHANNEL_0;
    case 1U: return ADC_CHANNEL_1;
    case 2U: return ADC_CHANNEL_2;
    case 3U: return ADC_CHANNEL_3;
    case 4U: return ADC_CHANNEL_4;
    case 5U: return ADC_CHANNEL_5;
    case 6U: return ADC_CHANNEL_6;
    case 7U: return ADC_CHANNEL_7;
    case 8U: return ADC_CHANNEL_8;
    case 9U: return ADC_CHANNEL_9;
    case 10U: return ADC_CHANNEL_10;
    case 11U: return ADC_CHANNEL_11;
    case 12U: return ADC_CHANNEL_12;
    case 13U: return ADC_CHANNEL_13;
    case 14U: return ADC_CHANNEL_14;
    case 15U: return ADC_CHANNEL_15;
    case 16U: return ADC_CHANNEL_16;
    case 17U: return ADC_CHANNEL_17;
    case 18U: return ADC_CHANNEL_18;
    case 19U: return ADC_CHANNEL_19;
    default: return 0U;
    }
}

void adc_gpio_init_for_default_channel(uint8_t channel)
{
    GPIO_InitTypeDef gpio = {};
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;

    switch (channel) {
    case 0U:
        __HAL_RCC_GPIOA_CLK_ENABLE();
        gpio.Pin = GPIO_PIN_0;
        HAL_GPIO_Init(GPIOA, &gpio);
        break;
    case 1U:
        __HAL_RCC_GPIOA_CLK_ENABLE();
        gpio.Pin = GPIO_PIN_1;
        HAL_GPIO_Init(GPIOA, &gpio);
        break;
    default:
        break;
    }
}

bool configure_adc1_dma(adc16_map_t &map, bool circular)
{
    if (map.base != ADC1) {
        return false;
    }

    if (s_adc1_dma_configured && (s_adc1_dma.Init.Mode == (circular ? DMA_CIRCULAR : DMA_NORMAL))) {
        map.handle.DMA_Handle = &s_adc1_dma;
        return true;
    }

    if (s_adc1_dma_configured) {
        (void) HAL_DMA_DeInit(&s_adc1_dma);
        s_adc1_dma_configured = false;
    }

    std::memset(&s_adc1_dma, 0, sizeof(s_adc1_dma));
    s_adc1_dma.Instance = DMA1_Stream5;
    s_adc1_dma.Init.Request = DMA_REQUEST_ADC1;
    s_adc1_dma.Init.Direction = DMA_PERIPH_TO_MEMORY;
    s_adc1_dma.Init.PeriphInc = DMA_PINC_DISABLE;
    s_adc1_dma.Init.MemInc = DMA_MINC_ENABLE;
    s_adc1_dma.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    s_adc1_dma.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    s_adc1_dma.Init.Mode = circular ? DMA_CIRCULAR : DMA_NORMAL;
    s_adc1_dma.Init.Priority = DMA_PRIORITY_HIGH;
    s_adc1_dma.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&s_adc1_dma) != HAL_OK) {
        return false;
    }

    map.handle.DMA_Handle = &s_adc1_dma;
    s_adc1_dma.Parent = &map.handle;
    HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
    s_adc1_dma_configured = true;
    return true;
}

void emit_sequence_dma_samples(adc16_map_t &map, const uint32_t *buffer, uint32_t sample_count)
{
    if ((buffer == nullptr) || (map.sequence_callback == nullptr) || (map.sequence_count == 0U)) {
        return;
    }

    const uint32_t groups = sample_count / map.sequence_count;
    for (uint32_t group = 0U; group < groups; ++group) {
        for (uint8_t sample_index = 0U; sample_index < map.sequence_count; ++sample_index) {
            const uint32_t buffer_index = (group * map.sequence_count) + sample_index;
            map.sequence_samples[sample_index].channel = map.sequence_channels[sample_index];
            map.sequence_samples[sample_index].index = sample_index;
            map.sequence_samples[sample_index].trigger_channel = 0U;
            map.sequence_samples[sample_index].value = static_cast<uint16_t>(buffer[buffer_index] & 0xFFFFU);
        }
        map.sequence_callback(map.sequence_samples, map.sequence_count, map.sequence_callback_arg);
    }
}

int32_t configure_channel_rank(adc16_map_t &map, uint8_t channel, uint8_t rank_index)
{
    const auto hal_channel = channel_to_hal(channel);
    if ((hal_channel == 0U) && (channel != 0U)) {
        return BSP_ERROR_PARAM;
    }

    ADC_ChannelConfTypeDef channel_cfg = {};
    channel_cfg.Channel = hal_channel;
    channel_cfg.Rank = rank_to_hal(rank_index);
    channel_cfg.SamplingTime = ADC_SAMPLETIME_387CYCLES_5;
    channel_cfg.SingleDiff = ADC_SINGLE_ENDED;
    channel_cfg.OffsetNumber = ADC_OFFSET_NONE;
    channel_cfg.Offset = 0;
    channel_cfg.OffsetSignedSaturation = DISABLE;

    return (HAL_ADC_ConfigChannel(&map.handle, &channel_cfg) == HAL_OK) ? BSP_OK : BSP_ERROR;
}

int32_t configure_injected_channel_rank(adc16_map_t &map,
                                        uint8_t channel,
                                        uint8_t rank_index,
                                        uint8_t count,
                                        bool hardware_trigger)
{
    const auto hal_channel = channel_to_hal(channel);
    if ((hal_channel == 0U) && (channel != 0U)) {
        return BSP_ERROR_PARAM;
    }

    ADC_InjectionConfTypeDef injected = {};
    injected.InjectedChannel = hal_channel;
    injected.InjectedRank = injected_rank_to_hal(rank_index);
    injected.InjectedSamplingTime = ADC_SAMPLETIME_387CYCLES_5;
    injected.InjectedSingleDiff = ADC_SINGLE_ENDED;
    injected.InjectedOffsetNumber = ADC_OFFSET_NONE;
    injected.InjectedOffset = 0U;
    injected.InjectedOffsetRightShift = DISABLE;
#if defined(ADC_VER_V5_V90)
    injected.InjectedOffsetSign = 0U;
    injected.InjectedOffsetSaturation = DISABLE;
#endif
    injected.InjectedOffsetSignedSaturation = DISABLE;
    injected.InjectedNbrOfConversion = count;
    injected.InjectedDiscontinuousConvMode = DISABLE;
    injected.AutoInjectedConv = DISABLE;
    injected.QueueInjectedContext = DISABLE;
    injected.ExternalTrigInjecConv = hardware_trigger ? map.preemption_external_trigger : ADC_INJECTED_SOFTWARE_START;
    injected.ExternalTrigInjecConvEdge = hardware_trigger ? map.preemption_external_trigger_edge : ADC_EXTERNALTRIGINJECCONV_EDGE_NONE;
    injected.InjecOversamplingMode = DISABLE;

    return (HAL_ADCEx_InjectedConfigChannel(&map.handle, &injected) == HAL_OK) ? BSP_OK : BSP_ERROR;
}

void fill_init(ADC_HandleTypeDef &handle,
               ADC_TypeDef *base,
               const bsp_adc16_init_config_t &config,
               uint8_t conversion_count,
               bool hardware_trigger,
               uint32_t external_trigger,
               uint32_t external_trigger_edge)
{
    handle.Instance = base;
    handle.Init.ClockPrescaler = divider_to_hal(config.clock_divider);
    handle.Init.Resolution = resolution_to_hal(config.resolution);
    handle.Init.ScanConvMode = (conversion_count > 1U) ? ADC_SCAN_ENABLE : ADC_SCAN_DISABLE;
    handle.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    handle.Init.LowPowerAutoWait = DISABLE;
    handle.Init.ContinuousConvMode = DISABLE;
    handle.Init.NbrOfConversion = conversion_count;
    handle.Init.DiscontinuousConvMode = DISABLE;
    handle.Init.ExternalTrigConv = hardware_trigger ? external_trigger : ADC_SOFTWARE_START;
    handle.Init.ExternalTrigConvEdge = hardware_trigger ? external_trigger_edge : ADC_EXTERNALTRIGCONVEDGE_NONE;
    handle.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DR;
    handle.Init.Overrun = ADC_OVR_DATA_PRESERVED;
    handle.Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
    handle.Init.OversamplingMode = DISABLE;
#if defined(ADC_VER_V5_V90)
    handle.Init.SamplingMode = ADC_SAMPLING_MODE_NORMAL;
#endif
}

int32_t apply_adc_init(adc16_map_t &map,
                       const bsp_adc16_init_config_t &config,
                       uint8_t conversion_count,
                       bool hardware_trigger = false)
{
    if (!is_supported_config(config) || (conversion_count == 0U) || (conversion_count > kMaxSequenceChannels)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (hardware_trigger) {
        if (!map.has_sequence_trigger_route || !is_supported_external_trigger(map.sequence_external_trigger)) {
            return BSP_ERROR_PARAM;
        }
    }

    __HAL_RCC_ADC12_CLK_ENABLE();
    adc_gpio_init_for_default_channel(map.default_channel);

    if (map.initialized) {
        (void) HAL_ADC_Stop(&map.handle);
        (void) HAL_ADC_Stop_IT(&map.handle);
        if (map.handle.DMA_Handle != nullptr) {
            (void) HAL_ADC_Stop_DMA(&map.handle);
        }
        (void) HAL_ADCEx_InjectedStop_IT(&map.handle);
        (void) HAL_ADC_DeInit(&map.handle);
    }

    fill_init(map.handle,
              map.base,
              config,
              conversion_count,
              hardware_trigger,
              map.sequence_external_trigger,
              map.sequence_external_trigger_edge);
    if (HAL_ADC_Init(&map.handle) != HAL_OK) {
        map.initialized = false;
        return BSP_ERROR;
    }
    if (HAL_ADCEx_Calibration_Start(&map.handle, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED) != HAL_OK) {
        map.initialized = false;
        return BSP_ERROR;
    }

    map.config = config;
    map.initialized = true;
    map.sequence_dma_active = false;
    map.sequence_dma_buffer = nullptr;
    map.sequence_dma_sample_count = 0U;
    HAL_NVIC_SetPriority(map.irq, 5, 0);
    HAL_NVIC_EnableIRQ(map.irq);
    return BSP_OK;
}

int32_t ensure_initialized(bsp_adc16_instance_t adc)
{
    if (adc >= BSP_ADC16_MAX) {
        return BSP_ERROR_PARAM;
    }

    auto &map = s_adc16_map[adc];
    if (!map.has_pinmux) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (!adc16_has_feature(map, BSP_ADC16_FEATURE_ONESHOT)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (map.initialized) {
        return BSP_OK;
    }

    bsp_adc16_init_config_t config{};
    bsp_adc16_get_default_init_config(&config);
    const auto init_status = apply_adc_init(map, config, 1U);
    if (init_status != BSP_OK) {
        return init_status;
    }
    map.active_channel = map.default_channel;
    return configure_channel_rank(map, map.default_channel, 0U);
}

} // namespace

void bsp_adc16_get_default_init_config(bsp_adc16_init_config_t *config)
{
    if (config == nullptr) {
        return;
    }
    config->resolution = BSP_ADC16_RESOLUTION_16_BITS;
    config->clock_divider = BSP_ADC16_CLOCK_DIVIDER_4;
}

int32_t bsp_adc16_init(bsp_adc16_instance_t adc)
{
    return ensure_initialized(adc);
}

int32_t bsp_adc16_init_ex(bsp_adc16_instance_t adc, const bsp_adc16_init_config_t *config)
{
    if ((adc >= BSP_ADC16_MAX) || (config == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    auto &map = s_adc16_map[adc];
    if (!map.has_pinmux) {
        return BSP_ERROR_UNSUPPORTED;
    }
    const auto config_status = validate_adc16_config_features(map, *config);
    if (config_status != BSP_OK) {
        return config_status;
    }

    const auto init_status = apply_adc_init(map, *config, 1U);
    if (init_status != BSP_OK) {
        return init_status;
    }
    map.active_channel = map.default_channel;
    return configure_channel_rank(map, map.default_channel, 0U);
}

int32_t bsp_adc16_get_clock(bsp_adc16_instance_t adc, uint32_t *freq_hz)
{
    if ((adc >= BSP_ADC16_MAX) || (freq_hz == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    if (!adc16_has_feature(s_adc16_map[adc], BSP_ADC16_FEATURE_GET_CLOCK)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    auto clock = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_ADC);
    if (clock == 0U) {
        clock = HAL_RCC_GetPCLK2Freq();
    }
    *freq_hz = clock;
    return BSP_OK;
}

int32_t bsp_adc16_read_channel(bsp_adc16_instance_t adc, uint8_t channel, uint16_t *value)
{
    if ((adc >= BSP_ADC16_MAX) || (value == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    const auto init_status = ensure_initialized(adc);
    if (init_status != BSP_OK) {
        return init_status;
    }

    auto &map = s_adc16_map[adc];
    if (!adc16_has_feature(map, BSP_ADC16_FEATURE_BLOCKING_READ)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (configure_channel_rank(map, channel, 0U) != BSP_OK) {
        return BSP_ERROR_PARAM;
    }
    map.active_channel = channel;
    map.sequence_active = false;

    if (HAL_ADC_Start(&map.handle) != HAL_OK) {
        return BSP_ERROR;
    }
    if (HAL_ADC_PollForConversion(&map.handle, HAL_MAX_DELAY) != HAL_OK) {
        (void) HAL_ADC_Stop(&map.handle);
        return BSP_ERROR_TIMEOUT;
    }
    *value = static_cast<uint16_t>(HAL_ADC_GetValue(&map.handle));
    (void) HAL_ADC_Stop(&map.handle);
    return BSP_OK;
}

int32_t bsp_adc16_read_default(bsp_adc16_instance_t adc, uint16_t *value)
{
    if ((adc >= BSP_ADC16_MAX) || (value == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    return bsp_adc16_read_channel(adc, s_adc16_map[adc].default_channel, value);
}

int32_t bsp_adc16_get_default_channel(bsp_adc16_instance_t adc, uint8_t *channel)
{
    if ((adc >= BSP_ADC16_MAX) || (channel == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    *channel = s_adc16_map[adc].default_channel;
    return BSP_OK;
}

int32_t bsp_adc16_configure_period(bsp_adc16_instance_t adc, uint8_t channel, uint8_t prescale, uint8_t period_count)
{
    if (adc >= BSP_ADC16_MAX) {
        return BSP_ERROR_PARAM;
    }
    if (!adc16_has_feature(s_adc16_map[adc], BSP_ADC16_FEATURE_PERIOD)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    (void) channel;
    (void) prescale;
    (void) period_count;
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_adc16_read_period(bsp_adc16_instance_t adc, uint8_t channel, uint16_t *value)
{
    if ((adc >= BSP_ADC16_MAX) || (value == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    if (!adc16_has_feature(s_adc16_map[adc], BSP_ADC16_FEATURE_PERIOD)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    (void) channel;
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_adc16_read_channel_async(bsp_adc16_instance_t adc, uint8_t channel)
{
    if (adc >= BSP_ADC16_MAX) {
        return BSP_ERROR_PARAM;
    }
    const auto init_status = ensure_initialized(adc);
    if (init_status != BSP_OK) {
        return init_status;
    }

    auto &map = s_adc16_map[adc];
    if (!adc16_has_feature(map, BSP_ADC16_FEATURE_INTERRUPT_READ)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (map.callback == nullptr) {
        return BSP_ERROR_PARAM;
    }
    if (configure_channel_rank(map, channel, 0U) != BSP_OK) {
        return BSP_ERROR_PARAM;
    }
    map.active_channel = channel;
    map.sequence_active = false;
    return (HAL_ADC_Start_IT(&map.handle) == HAL_OK) ? BSP_OK : BSP_ERROR_BUSY;
}

int32_t bsp_adc16_read_default_async(bsp_adc16_instance_t adc)
{
    if (adc >= BSP_ADC16_MAX) {
        return BSP_ERROR_PARAM;
    }
    return bsp_adc16_read_channel_async(adc, s_adc16_map[adc].default_channel);
}

int32_t bsp_adc16_register_callback(bsp_adc16_instance_t adc, bsp_adc16_callback_t callback, void *arg)
{
    if (adc >= BSP_ADC16_MAX) {
        return BSP_ERROR_PARAM;
    }
    if (!adc16_has_feature(s_adc16_map[adc], BSP_ADC16_FEATURE_INTERRUPT_READ)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    s_adc16_map[adc].callback = callback;
    s_adc16_map[adc].callback_arg = arg;
    return BSP_OK;
}

int32_t bsp_adc16_set_sequence_trigger_route(bsp_adc16_instance_t adc, const bsp_adc16_trigger_route_t *route)
{
    if ((adc >= BSP_ADC16_MAX) || (route == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    if (!adc16_has_feature(s_adc16_map[adc], BSP_ADC16_FEATURE_TRIGGER_ROUTE) ||
        !adc16_has_feature(s_adc16_map[adc], BSP_ADC16_FEATURE_SEQUENCE_HARDWARE_TRIGGER)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (!is_supported_external_trigger(route->input) || (route->input == ADC_SOFTWARE_START)) {
        return BSP_ERROR_PARAM;
    }

    auto &map = s_adc16_map[adc];
    map.sequence_external_trigger = route->input;
    map.sequence_external_trigger_edge = trigger_route_edge(*route);
    map.has_sequence_trigger_route = true;
    return BSP_OK;
}

int32_t bsp_adc16_configure_sequence(bsp_adc16_instance_t adc,
                                     const uint8_t *channels,
                                     uint8_t count,
                                     bsp_state_t continuous,
                                     bsp_state_t hardware_trigger)
{
    if ((adc >= BSP_ADC16_MAX) || (channels == nullptr) || (count == 0U) || (count > kMaxSequenceChannels)) {
        return BSP_ERROR_PARAM;
    }
    auto &map = s_adc16_map[adc];
    if (!map.has_pinmux) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (!adc16_has_feature(map, BSP_ADC16_FEATURE_SEQUENCE)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    const bool use_hardware_trigger = (hardware_trigger == BSP_ENABLE);
    if (use_hardware_trigger && !adc16_has_feature(map, BSP_ADC16_FEATURE_SEQUENCE_HARDWARE_TRIGGER)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (use_hardware_trigger && !map.has_sequence_trigger_route) {
        return BSP_ERROR_PARAM;
    }

    const auto config = map.initialized ? map.config : bsp_adc16_init_config_t{BSP_ADC16_RESOLUTION_16_BITS, BSP_ADC16_CLOCK_DIVIDER_4};
    const auto init_status = apply_adc_init(map, config, count, use_hardware_trigger);
    if (init_status != BSP_OK) {
        return init_status;
    }

    for (uint8_t index = 0U; index < count; ++index) {
        if (configure_channel_rank(map, channels[index], index) != BSP_OK) {
            return BSP_ERROR_PARAM;
        }
        map.sequence_channels[index] = channels[index];
    }

    map.sequence_count = count;
    map.sequence_index = 0U;
    map.sequence_continuous = (continuous == BSP_ENABLE);
    map.sequence_hardware_trigger = use_hardware_trigger;
    map.sequence_dma_active = false;
    map.preemption_active = false;
    return BSP_OK;
}

int32_t bsp_adc16_start_sequence(bsp_adc16_instance_t adc)
{
    if (adc >= BSP_ADC16_MAX) {
        return BSP_ERROR_PARAM;
    }
    auto &map = s_adc16_map[adc];
    if (!adc16_has_feature(map, BSP_ADC16_FEATURE_SEQUENCE)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (!map.initialized || (map.sequence_count == 0U) || (map.sequence_callback == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    map.sequence_active = true;
    map.sequence_dma_active = false;
    map.sequence_index = 0U;
    std::memset(map.sequence_samples, 0, sizeof(map.sequence_samples));
    return (HAL_ADC_Start_IT(&map.handle) == HAL_OK) ? BSP_OK : BSP_ERROR_BUSY;
}

int32_t bsp_adc16_start_sequence_dma(bsp_adc16_instance_t adc, const bsp_adc16_dma_sequence_config_t *config)
{
    if ((adc >= BSP_ADC16_MAX) || (config == nullptr) || (config->buffer == nullptr) || (config->sample_count == 0U)) {
        return BSP_ERROR_PARAM;
    }
    auto &map = s_adc16_map[adc];
    if (!adc16_has_feature(map, BSP_ADC16_FEATURE_SEQUENCE_DMA)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if ((config->circular == BSP_ENABLE) && !adc16_has_feature(map, BSP_ADC16_FEATURE_CONTINUOUS_DMA)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (!map.initialized || (map.sequence_count == 0U) || (map.sequence_callback == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    if ((config->sample_count % map.sequence_count) != 0U) {
        return BSP_ERROR_PARAM;
    }

    const bool circular = (config->circular == BSP_ENABLE);
    if (!configure_adc1_dma(map, circular)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    map.handle.Init.ConversionDataManagement = circular ? ADC_CONVERSIONDATA_DMA_CIRCULAR : ADC_CONVERSIONDATA_DMA_ONESHOT;
    if (HAL_ADC_Init(&map.handle) != HAL_OK) {
        return BSP_ERROR;
    }
    for (uint8_t index = 0U; index < map.sequence_count; ++index) {
        if (configure_channel_rank(map, map.sequence_channels[index], index) != BSP_OK) {
            return BSP_ERROR_PARAM;
        }
    }

    map.sequence_active = true;
    map.sequence_dma_active = true;
    map.sequence_dma_circular = circular;
    map.sequence_dma_buffer = config->buffer;
    map.sequence_dma_sample_count = config->sample_count;
    map.sequence_index = 0U;
    std::memset(map.sequence_samples, 0, sizeof(map.sequence_samples));
    return (HAL_ADC_Start_DMA(&map.handle, config->buffer, config->sample_count) == HAL_OK) ? BSP_OK : BSP_ERROR_BUSY;
}

int32_t bsp_adc16_register_sequence_callback(bsp_adc16_instance_t adc, bsp_adc16_sequence_callback_t callback, void *arg)
{
    if (adc >= BSP_ADC16_MAX) {
        return BSP_ERROR_PARAM;
    }
    if (!adc16_has_feature(s_adc16_map[adc], BSP_ADC16_FEATURE_SEQUENCE)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    s_adc16_map[adc].sequence_callback = callback;
    s_adc16_map[adc].sequence_callback_arg = arg;
    return BSP_OK;
}

int32_t bsp_adc16_set_preemption_trigger_route(bsp_adc16_instance_t adc, const bsp_adc16_trigger_route_t *route)
{
    if ((adc >= BSP_ADC16_MAX) || (route == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    if (!adc16_has_feature(s_adc16_map[adc], BSP_ADC16_FEATURE_TRIGGER_ROUTE) ||
        !adc16_has_feature(s_adc16_map[adc], BSP_ADC16_FEATURE_PREEMPTION_HARDWARE_TRIGGER)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (!is_supported_injected_external_trigger(route->input) || (route->input == ADC_INJECTED_SOFTWARE_START)) {
        return BSP_ERROR_PARAM;
    }

    auto &map = s_adc16_map[adc];
    map.preemption_external_trigger = route->input;
    map.preemption_external_trigger_edge = injected_trigger_route_edge(*route);
    map.has_preemption_trigger_route = true;
    return BSP_OK;
}

int32_t bsp_adc16_configure_preemption(bsp_adc16_instance_t adc,
                                       uint8_t trigger_channel,
                                       const uint8_t *channels,
                                       uint8_t count,
                                       bsp_state_t hardware_trigger)
{
    if ((adc >= BSP_ADC16_MAX) || (channels == nullptr) || (count == 0U) || (count > kMaxInjectedChannels)) {
        return BSP_ERROR_PARAM;
    }
    auto &map = s_adc16_map[adc];
    if (!map.has_pinmux) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (!adc16_has_feature(map, BSP_ADC16_FEATURE_PREEMPTION)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    const bool use_hardware_trigger = (hardware_trigger == BSP_ENABLE);
    if (use_hardware_trigger && !adc16_has_feature(map, BSP_ADC16_FEATURE_PREEMPTION_HARDWARE_TRIGGER)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (use_hardware_trigger) {
        if (!map.has_preemption_trigger_route || !is_supported_injected_external_trigger(map.preemption_external_trigger)) {
            return BSP_ERROR_PARAM;
        }
    }

    const auto config = map.initialized ? map.config : bsp_adc16_init_config_t{BSP_ADC16_RESOLUTION_16_BITS, BSP_ADC16_CLOCK_DIVIDER_4};
    const auto init_status = apply_adc_init(map, config, count, false);
    if (init_status != BSP_OK) {
        return init_status;
    }

    for (uint8_t index = 0U; index < count; ++index) {
        if (configure_injected_channel_rank(map, channels[index], index, count, use_hardware_trigger) != BSP_OK) {
            return BSP_ERROR_PARAM;
        }
        map.preemption_channels[index] = channels[index];
    }

    map.preemption_trigger_channel = trigger_channel;
    map.preemption_count = count;
    map.preemption_hardware_trigger = use_hardware_trigger;
    map.preemption_active = false;
    map.sequence_active = false;
    return BSP_OK;
}

int32_t bsp_adc16_start_preemption(bsp_adc16_instance_t adc)
{
    if (adc >= BSP_ADC16_MAX) {
        return BSP_ERROR_PARAM;
    }
    auto &map = s_adc16_map[adc];
    if (!adc16_has_feature(map, BSP_ADC16_FEATURE_PREEMPTION)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (!map.initialized || (map.preemption_count == 0U) || (map.preemption_callback == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    map.preemption_active = true;
    std::memset(map.preemption_samples, 0, sizeof(map.preemption_samples));
    return (HAL_ADCEx_InjectedStart_IT(&map.handle) == HAL_OK) ? BSP_OK : BSP_ERROR_BUSY;
}

int32_t bsp_adc16_register_preemption_callback(bsp_adc16_instance_t adc, bsp_adc16_preemption_callback_t callback, void *arg)
{
    if (adc >= BSP_ADC16_MAX) {
        return BSP_ERROR_PARAM;
    }
    if (!adc16_has_feature(s_adc16_map[adc], BSP_ADC16_FEATURE_PREEMPTION)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    s_adc16_map[adc].preemption_callback = callback;
    s_adc16_map[adc].preemption_callback_arg = arg;
    return BSP_OK;
}

extern "C" void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    for (std::uint32_t index = 0; index < BSP_ADC16_MAX; ++index) {
        auto &map = s_adc16_map[index];
        if (&map.handle != hadc) {
            continue;
        }

        if (map.sequence_dma_active) {
            if ((map.sequence_dma_buffer != nullptr) && (map.sequence_dma_sample_count > 0U)) {
                const uint32_t offset = map.sequence_dma_circular ? (map.sequence_dma_sample_count / 2U) : 0U;
                const uint32_t count = map.sequence_dma_circular ? (map.sequence_dma_sample_count - offset) : map.sequence_dma_sample_count;
                emit_sequence_dma_samples(map, map.sequence_dma_buffer + offset, count);
            }
            if (!map.sequence_dma_circular) {
                map.sequence_active = false;
                map.sequence_dma_active = false;
                (void) HAL_ADC_Stop_DMA(hadc);
            }
            break;
        }

        const auto value = static_cast<uint16_t>(HAL_ADC_GetValue(hadc));
        if (map.sequence_active) {
            const auto sample_index = map.sequence_index;
            if (sample_index < map.sequence_count) {
                map.sequence_samples[sample_index].channel = map.sequence_channels[sample_index];
                map.sequence_samples[sample_index].index = sample_index;
                map.sequence_samples[sample_index].trigger_channel = 0U;
                map.sequence_samples[sample_index].value = value;
                ++map.sequence_index;
            }
            if (map.sequence_index >= map.sequence_count) {
                if (!map.sequence_continuous) {
                    map.sequence_active = false;
                    (void) HAL_ADC_Stop_IT(hadc);
                } else {
                    map.sequence_index = 0U;
                }
                if (map.sequence_callback != nullptr) {
                    map.sequence_callback(map.sequence_samples, map.sequence_count, map.sequence_callback_arg);
                }
            }
        } else if (map.callback != nullptr) {
            (void) HAL_ADC_Stop_IT(hadc);
            map.callback(map.active_channel, value, map.callback_arg);
        }
        break;
    }
}

extern "C" void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
    for (std::uint32_t index = 0; index < BSP_ADC16_MAX; ++index) {
        auto &map = s_adc16_map[index];
        if (&map.handle != hadc) {
            continue;
        }
        if (map.sequence_dma_active && map.sequence_dma_circular && (map.sequence_dma_buffer != nullptr)) {
            emit_sequence_dma_samples(map, map.sequence_dma_buffer, map.sequence_dma_sample_count / 2U);
        }
        break;
    }
}

extern "C" void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    for (std::uint32_t index = 0; index < BSP_ADC16_MAX; ++index) {
        auto &map = s_adc16_map[index];
        if (&map.handle != hadc) {
            continue;
        }

        if (map.preemption_active && (map.preemption_count > 0U)) {
            for (uint8_t sample_index = 0U; sample_index < map.preemption_count; ++sample_index) {
                map.preemption_samples[sample_index].channel = map.preemption_channels[sample_index];
                map.preemption_samples[sample_index].index = sample_index;
                map.preemption_samples[sample_index].trigger_channel = map.preemption_trigger_channel;
                map.preemption_samples[sample_index].value =
                    static_cast<uint16_t>(HAL_ADCEx_InjectedGetValue(hadc, injected_rank_to_hal(sample_index)));
            }
            if (!map.preemption_hardware_trigger) {
                map.preemption_active = false;
                (void) HAL_ADCEx_InjectedStop_IT(hadc);
            }
            if (map.preemption_callback != nullptr) {
                map.preemption_callback(map.preemption_trigger_channel,
                                        map.preemption_samples,
                                        map.preemption_count,
                                        map.preemption_callback_arg);
            }
        }
        break;
    }
}

extern "C" void DMA1_Stream5_IRQHandler(void)
{
    if (s_adc1_dma_configured) {
        HAL_DMA_IRQHandler(&s_adc1_dma);
    }
}

extern "C" void bsp_stm32_adc_irq_handler(void)
{
    if (s_adc16_map[BSP_ADC16_MAIN].initialized) {
        HAL_ADC_IRQHandler(&s_adc16_map[BSP_ADC16_MAIN].handle);
    }
}

extern "C" void ADC_IRQHandler(void)
{
    bsp_stm32_adc_irq_handler();
}

#else

void bsp_adc16_get_default_init_config(bsp_adc16_init_config_t *config)
{
    if (config == nullptr) {
        return;
    }
    config->resolution = BSP_ADC16_RESOLUTION_16_BITS;
    config->clock_divider = BSP_ADC16_CLOCK_DIVIDER_4;
}

int32_t bsp_adc16_init(bsp_adc16_instance_t adc)
{
    if (adc >= BSP_ADC16_MAX) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_adc16_init_ex(bsp_adc16_instance_t adc, const bsp_adc16_init_config_t *config)
{
    if ((adc >= BSP_ADC16_MAX) || (config == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_adc16_get_clock(bsp_adc16_instance_t adc, uint32_t *freq_hz)
{
    if ((adc >= BSP_ADC16_MAX) || (freq_hz == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    *freq_hz = 0U;
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_adc16_read_channel(bsp_adc16_instance_t adc, uint8_t channel, uint16_t *value)
{
    (void) channel;
    if ((adc >= BSP_ADC16_MAX) || (value == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    *value = 0U;
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_adc16_read_default(bsp_adc16_instance_t adc, uint16_t *value)
{
    if ((adc >= BSP_ADC16_MAX) || (value == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    *value = 0U;
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_adc16_get_default_channel(bsp_adc16_instance_t adc, uint8_t *channel)
{
    if ((adc >= BSP_ADC16_MAX) || (channel == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    *channel = 0U;
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_adc16_configure_period(bsp_adc16_instance_t adc, uint8_t channel, uint8_t prescale, uint8_t period_count)
{
    (void) channel;
    (void) prescale;
    (void) period_count;
    if (adc >= BSP_ADC16_MAX) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_adc16_read_period(bsp_adc16_instance_t adc, uint8_t channel, uint16_t *value)
{
    (void) channel;
    if ((adc >= BSP_ADC16_MAX) || (value == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    *value = 0U;
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_adc16_read_channel_async(bsp_adc16_instance_t adc, uint8_t channel)
{
    (void) channel;
    if (adc >= BSP_ADC16_MAX) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_adc16_read_default_async(bsp_adc16_instance_t adc)
{
    if (adc >= BSP_ADC16_MAX) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_adc16_register_callback(bsp_adc16_instance_t adc, bsp_adc16_callback_t callback, void *arg)
{
    (void) callback;
    (void) arg;
    if (adc >= BSP_ADC16_MAX) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_adc16_set_sequence_trigger_route(bsp_adc16_instance_t adc, const bsp_adc16_trigger_route_t *route)
{
    if ((adc >= BSP_ADC16_MAX) || (route == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_adc16_configure_sequence(bsp_adc16_instance_t adc,
                                     const uint8_t *channels,
                                     uint8_t count,
                                     bsp_state_t continuous,
                                     bsp_state_t hardware_trigger)
{
    (void) continuous;
    (void) hardware_trigger;
    if ((adc >= BSP_ADC16_MAX) || (channels == nullptr) || (count == 0U)) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_adc16_start_sequence(bsp_adc16_instance_t adc)
{
    if (adc >= BSP_ADC16_MAX) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_adc16_start_sequence_dma(bsp_adc16_instance_t adc, const bsp_adc16_dma_sequence_config_t *config)
{
    if ((adc >= BSP_ADC16_MAX) || (config == nullptr) || (config->buffer == nullptr) || (config->sample_count == 0U)) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_adc16_register_sequence_callback(bsp_adc16_instance_t adc, bsp_adc16_sequence_callback_t callback, void *arg)
{
    (void) callback;
    (void) arg;
    if (adc >= BSP_ADC16_MAX) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_adc16_set_preemption_trigger_route(bsp_adc16_instance_t adc, const bsp_adc16_trigger_route_t *route)
{
    if ((adc >= BSP_ADC16_MAX) || (route == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_adc16_configure_preemption(bsp_adc16_instance_t adc,
                                       uint8_t trigger_channel,
                                       const uint8_t *channels,
                                       uint8_t count,
                                       bsp_state_t hardware_trigger)
{
    (void) trigger_channel;
    (void) hardware_trigger;
    if ((adc >= BSP_ADC16_MAX) || (channels == nullptr) || (count == 0U)) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_adc16_start_preemption(bsp_adc16_instance_t adc)
{
    if (adc >= BSP_ADC16_MAX) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_adc16_register_preemption_callback(bsp_adc16_instance_t adc, bsp_adc16_preemption_callback_t callback, void *arg)
{
    (void) callback;
    (void) arg;
    if (adc >= BSP_ADC16_MAX) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR_UNSUPPORTED;
}

extern "C" void bsp_stm32_adc_irq_handler(void)
{
}

extern "C" void ADC_IRQHandler(void)
{
    bsp_stm32_adc_irq_handler();
}

#endif

/**
 * @file bsp_adc16_hpm.cpp
 * @brief HPM platform implementation for BSP ADC16 interface
 */

#include "bsp_adc16.h"
#include "board.h"

extern "C" {
#include "hpm_adc16_drv.h"
#include "hpm_interrupt.h"
#include "hpm_trgm_drv.h"
}

typedef struct {
    ADC16_Type *base;
    uint32_t irq;
    uint32_t clock_name;
    uint32_t clock_hz;
    uint8_t default_channel;
    uint32_t features;
    bsp_adc16_resolution_t resolution;
    bsp_adc16_clock_divider_t clock_divider;
    bool clk_src_bus;
    bool has_pinmux;
    bool initialized;
    uint8_t conversion_mode;
    bool async_started;
    bsp_adc16_callback_t callback;
    void *callback_arg;
    uint8_t async_channel;
    uint8_t period_channel;
    uint8_t sequence_len;
    bool sequence_continuous;
    bool sequence_hw_trigger;
    bool sequence_configured;
    bool sequence_route_configured;
    bsp_adc16_trigger_route_t sequence_route;
    uint8_t sequence_channels[ADC_SOC_SEQ_MAX_LEN];
    bsp_adc16_sequence_callback_t sequence_callback;
    void *sequence_callback_arg;
    uint8_t preemption_trigger_channel;
    uint8_t preemption_len;
    bool preemption_hw_trigger;
    bool preemption_configured;
    bool preemption_route_configured;
    bsp_adc16_trigger_route_t preemption_route;
    uint8_t preemption_channels[ADC_SOC_MAX_TRIG_CH_LEN];
    bsp_adc16_preemption_callback_t preemption_callback;
    void *preemption_callback_arg;
    bsp_adc16_sample_t sequence_samples[ADC_SOC_SEQ_MAX_LEN];
    bsp_adc16_sample_t preemption_samples[ADC_SOC_MAX_TRIG_CH_LEN];
} adc16_instance_map_t;

namespace {

static adc16_instance_map_t make_adc16_map(ADC16_Type *base,
                                           uint32_t irq,
                                           uint32_t clock_name,
                                           uint8_t default_channel,
                                           uint32_t clk_src_bus,
                                           uint32_t has_pinmux,
                                           uint32_t features)
{
    adc16_instance_map_t map = {};
    map.base = base;
    map.irq = irq;
    map.clock_name = clock_name;
    map.default_channel = default_channel;
    map.features = features;
    map.resolution = BSP_ADC16_RESOLUTION_DEFAULT;
    map.clock_divider = BSP_ADC16_CLOCK_DIVIDER_DEFAULT;
    map.clk_src_bus = (clk_src_bus != 0U);
    map.has_pinmux = (has_pinmux != 0U);
    map.conversion_mode = adc16_conv_mode_oneshot;
    map.async_channel = default_channel;
    map.period_channel = default_channel;
    return map;
}

} // namespace

#define BSP_ADC16_MAP_ITEM(name, base_addr, irq_num, clk_name, default_channel, clk_src_bus, has_pinmux, features) \
    make_adc16_map(base_addr, irq_num, clk_name, default_channel, clk_src_bus, has_pinmux, static_cast<uint32_t>(features)),
static adc16_instance_map_t s_adc16_map[BSP_ADC16_MAX] = {
    BSP_ADC16_LIST(BSP_ADC16_MAP_ITEM)
};
#undef BSP_ADC16_MAP_ITEM

ATTR_PLACE_AT_NONCACHEABLE_WITH_ALIGNMENT(ADC_SOC_DMA_ADDR_ALIGNMENT)
static uint32_t s_adc16_seq_dma_buf[BSP_ADC16_MAX][ADC_SOC_SEQ_MAX_LEN];

ATTR_PLACE_AT_NONCACHEABLE_WITH_ALIGNMENT(ADC_SOC_DMA_ADDR_ALIGNMENT)
static uint32_t s_adc16_pmt_dma_buf[BSP_ADC16_MAX][ADC_SOC_PMT_MAX_DMA_BUFF_LEN_IN_4BYTES];

namespace {

constexpr uint8_t k_pmt_trigger_stride = ADC_SOC_MAX_TRIG_CH_LEN;
constexpr uint32_t k_oneshot_retry_count = 1024U;

static bool adc16_is_valid_channel_count(uint8_t count)
{
    return (count > 0U) && (count <= ADC_SOC_SEQ_MAX_LEN);
}

static bool adc16_is_valid_preemption_count(uint8_t count)
{
    return (count > 0U) && (count <= ADC_SOC_MAX_TRIG_CH_LEN);
}

static bool adc16_has_feature(const adc16_instance_map_t &map, uint32_t feature)
{
    return (map.features & feature) != 0U;
}

static int32_t adc16_validate_config_features(const adc16_instance_map_t &map, const bsp_adc16_init_config_t &config)
{
    switch (config.resolution) {
    case BSP_ADC16_RESOLUTION_DEFAULT:
    case BSP_ADC16_RESOLUTION_8_BITS:
    case BSP_ADC16_RESOLUTION_10_BITS:
    case BSP_ADC16_RESOLUTION_12_BITS:
    case BSP_ADC16_RESOLUTION_16_BITS:
        break;
    default:
        return BSP_ERROR_UNSUPPORTED;
    }

    switch (config.clock_divider) {
    case BSP_ADC16_CLOCK_DIVIDER_DEFAULT:
    case BSP_ADC16_CLOCK_DIVIDER_1:
    case BSP_ADC16_CLOCK_DIVIDER_2:
    case BSP_ADC16_CLOCK_DIVIDER_3:
    case BSP_ADC16_CLOCK_DIVIDER_4:
    case BSP_ADC16_CLOCK_DIVIDER_5:
    case BSP_ADC16_CLOCK_DIVIDER_6:
    case BSP_ADC16_CLOCK_DIVIDER_7:
    case BSP_ADC16_CLOCK_DIVIDER_8:
    case BSP_ADC16_CLOCK_DIVIDER_9:
    case BSP_ADC16_CLOCK_DIVIDER_10:
    case BSP_ADC16_CLOCK_DIVIDER_11:
    case BSP_ADC16_CLOCK_DIVIDER_12:
    case BSP_ADC16_CLOCK_DIVIDER_13:
    case BSP_ADC16_CLOCK_DIVIDER_14:
    case BSP_ADC16_CLOCK_DIVIDER_15:
    case BSP_ADC16_CLOCK_DIVIDER_16:
        break;
    default:
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

static int32_t adc16_validate_conversion_mode(const adc16_instance_map_t &map, uint8_t conversion_mode)
{
    if (conversion_mode == adc16_conv_mode_oneshot) {
        return adc16_has_feature(map, BSP_ADC16_FEATURE_ONESHOT) ? BSP_OK : BSP_ERROR_UNSUPPORTED;
    }
    if (conversion_mode == adc16_conv_mode_period) {
        return adc16_has_feature(map, BSP_ADC16_FEATURE_PERIOD) ? BSP_OK : BSP_ERROR_UNSUPPORTED;
    }
    if (conversion_mode == adc16_conv_mode_sequence) {
        return adc16_has_feature(map, BSP_ADC16_FEATURE_SEQUENCE) ? BSP_OK : BSP_ERROR_UNSUPPORTED;
    }
    if (conversion_mode == adc16_conv_mode_preemption) {
        return adc16_has_feature(map, BSP_ADC16_FEATURE_PREEMPTION) ? BSP_OK : BSP_ERROR_UNSUPPORTED;
    }
    return BSP_ERROR_UNSUPPORTED;
}

static adc16_resolution_t adc16_resolution_to_hpm(bsp_adc16_resolution_t resolution)
{
    switch (resolution) {
    case BSP_ADC16_RESOLUTION_8_BITS:
        return adc16_res_8_bits;
    case BSP_ADC16_RESOLUTION_10_BITS:
        return adc16_res_10_bits;
    case BSP_ADC16_RESOLUTION_12_BITS:
        return adc16_res_12_bits;
    case BSP_ADC16_RESOLUTION_16_BITS:
    case BSP_ADC16_RESOLUTION_DEFAULT:
    default:
        return adc16_res_16_bits;
    }
}

static adc16_clock_divider_t adc16_clock_divider_to_hpm(bsp_adc16_clock_divider_t divider)
{
    switch (divider) {
    case BSP_ADC16_CLOCK_DIVIDER_1:
        return adc16_clock_divider_1;
    case BSP_ADC16_CLOCK_DIVIDER_2:
        return adc16_clock_divider_2;
    case BSP_ADC16_CLOCK_DIVIDER_3:
        return adc16_clock_divider_3;
    case BSP_ADC16_CLOCK_DIVIDER_4:
    case BSP_ADC16_CLOCK_DIVIDER_DEFAULT:
        return adc16_clock_divider_4;
    case BSP_ADC16_CLOCK_DIVIDER_5:
        return adc16_clock_divider_5;
    case BSP_ADC16_CLOCK_DIVIDER_6:
        return adc16_clock_divider_6;
    case BSP_ADC16_CLOCK_DIVIDER_7:
        return adc16_clock_divider_7;
    case BSP_ADC16_CLOCK_DIVIDER_8:
        return adc16_clock_divider_8;
    case BSP_ADC16_CLOCK_DIVIDER_9:
        return adc16_clock_divider_9;
    case BSP_ADC16_CLOCK_DIVIDER_10:
        return adc16_clock_divider_10;
    case BSP_ADC16_CLOCK_DIVIDER_11:
        return adc16_clock_divider_11;
    case BSP_ADC16_CLOCK_DIVIDER_12:
        return adc16_clock_divider_12;
    case BSP_ADC16_CLOCK_DIVIDER_13:
        return adc16_clock_divider_13;
    case BSP_ADC16_CLOCK_DIVIDER_14:
        return adc16_clock_divider_14;
    case BSP_ADC16_CLOCK_DIVIDER_15:
        return adc16_clock_divider_15;
    case BSP_ADC16_CLOCK_DIVIDER_16:
        return adc16_clock_divider_16;
    default:
        return adc16_clock_divider_4;
    }
}

static int32_t adc16_init_hpm_instance(adc16_instance_map_t *map, uint8_t conversion_mode)
{
    adc16_config_t config;
    uint32_t freq;

    if (!map->has_pinmux) {
        return BSP_ERROR_UNSUPPORTED;
    }
    const auto mode_status = adc16_validate_conversion_mode(*map, conversion_mode);
    if (mode_status != BSP_OK) {
        return mode_status;
    }

    freq = board_init_adc_clock((void *) map->base, map->clk_src_bus);
    if (freq == 0U) {
        return BSP_ERROR_UNSUPPORTED;
    }

    adc16_get_default_config(&config);
    config.res = adc16_resolution_to_hpm(map->resolution);
    config.conv_mode = conversion_mode;
    config.adc_clk_div = adc16_clock_divider_to_hpm(map->clock_divider);
    config.sel_sync_ahb = map->clk_src_bus;
    config.adc_ahb_en = (conversion_mode == adc16_conv_mode_sequence)
                     || (conversion_mode == adc16_conv_mode_preemption);
    if (adc16_init(map->base, &config) != status_success) {
        return BSP_ERROR;
    }

    if (conversion_mode == adc16_conv_mode_oneshot) {
        adc16_enable_oneshot_mode(map->base);
    }

#if defined(HPM_IP_FEATURE_ADC16_HAS_MOT_EN) && HPM_IP_FEATURE_ADC16_HAS_MOT_EN
    adc16_enable_motor(map->base);
#endif
    map->clock_hz = freq;
    map->initialized = true;
    map->conversion_mode = conversion_mode;
    map->async_started = false;
    map->period_channel = map->default_channel;
    map->sequence_configured = false;
    map->preemption_configured = false;
    return BSP_OK;
}

static int32_t ensure_initialized(adc16_instance_map_t *map, uint8_t conversion_mode)
{
    if (!map->initialized || (map->conversion_mode != conversion_mode)) {
        map->initialized = false;
        return adc16_init_hpm_instance(map, conversion_mode);
    }
    return BSP_OK;
}

static int32_t ensure_initialized(bsp_adc16_instance_t adc, uint8_t conversion_mode)
{
    if (adc >= BSP_ADC16_MAX) {
        return BSP_ERROR_PARAM;
    }
    return ensure_initialized(&s_adc16_map[adc], conversion_mode);
}

static int32_t adc16_configure_channel(ADC16_Type *base, uint8_t channel)
{
    adc16_channel_config_t ch_config;

#if defined(BSP_CHIP_HPM5301)
    switch (channel) {
    case 1U:
        HPM_IOC->PAD[IOC_PAD_PB09].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
        break;
    case 2U:
        HPM_IOC->PAD[IOC_PAD_PB10].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
        break;
    case 3U:
        HPM_IOC->PAD[IOC_PAD_PB11].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
        break;
    case 4U:
        HPM_IOC->PAD[IOC_PAD_PB12].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
        break;
    case 5U:
        HPM_IOC->PAD[IOC_PAD_PB13].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
        break;
    case 6U:
        HPM_IOC->PAD[IOC_PAD_PB14].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
        break;
    case 7U:
        HPM_IOC->PAD[IOC_PAD_PB15].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
        break;
    case 11U:
        HPM_IOC->PAD[IOC_PAD_PB08].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
        break;
    default:
        break;
    }
#else
    board_init_adc16_pins();
#endif
    adc16_get_channel_default_config(&ch_config);
    ch_config.ch = channel;
    ch_config.sample_cycle = 20U;
    return (adc16_init_channel(base, &ch_config) == status_success) ? BSP_OK : BSP_ERROR;
}

static bool adc16_resolve_sequence_trigger_route(TRGM_Type **trgm_base, uint8_t *input, uint8_t *output)
{
#if defined(BOARD_APP_ADC16_HW_TRGM) && defined(BOARD_APP_ADC16_HW_TRGM_IN) && defined(BOARD_APP_ADC16_HW_TRGM_OUT_SEQ)
    *trgm_base = BOARD_APP_ADC16_HW_TRGM;
    *input = BOARD_APP_ADC16_HW_TRGM_IN;
    *output = BOARD_APP_ADC16_HW_TRGM_OUT_SEQ;
    return true;
#else
    (void) trgm_base;
    (void) input;
    (void) output;
    return false;
#endif
}

static bool adc16_resolve_preemption_trigger_route(TRGM_Type **trgm_base, uint8_t *input, uint8_t *output)
{
#if defined(BOARD_APP_ADC16_HW_TRGM) && defined(BOARD_APP_ADC16_HW_TRGM_IN) && defined(BOARD_APP_ADC16_HW_TRGM_OUT_PMT)
    *trgm_base = BOARD_APP_ADC16_HW_TRGM;
    *input = BOARD_APP_ADC16_HW_TRGM_IN;
    *output = BOARD_APP_ADC16_HW_TRGM_OUT_PMT;
    return true;
#else
    (void) trgm_base;
    (void) input;
    (void) output;
    return false;
#endif
}

static trgm_output_type_t adc16_trigger_output_type_to_hpm(bsp_adc16_trigger_output_t type)
{
    switch (type) {
    case BSP_ADC16_TRIGGER_OUTPUT_PULSE_FALLING:
        return trgm_output_pulse_at_input_falling_edge;
    case BSP_ADC16_TRIGGER_OUTPUT_PULSE_RISING:
        return trgm_output_pulse_at_input_rising_edge;
    case BSP_ADC16_TRIGGER_OUTPUT_PULSE_BOTH:
        return trgm_output_pulse_at_input_both_edge;
    case BSP_ADC16_TRIGGER_OUTPUT_SAME_AS_INPUT:
    default:
        return trgm_output_same_as_input;
    }
}

static int32_t adc16_configure_trigger_route(adc16_instance_map_t *map, bool is_preemption)
{
    TRGM_Type *trgm_base = nullptr;
    uint8_t input = 0U;
    uint8_t output = 0U;
    trgm_output_t trgm_output_cfg = {};
    const bsp_adc16_trigger_route_t *route = nullptr;

    if (is_preemption && map->preemption_route_configured) {
        route = &map->preemption_route;
    } else if (!is_preemption && map->sequence_route_configured) {
        route = &map->sequence_route;
    }

    if (route != nullptr) {
        trgm_base = HPM_TRGM0;
        input = route->input;
        output = route->output;
        trgm_output_cfg.invert = (route->invert == BSP_ENABLE);
        trgm_output_cfg.type = adc16_trigger_output_type_to_hpm(route->type);
        trgm_output_cfg.input = static_cast<uint8_t>(input);
        trgm_output_config(trgm_base, static_cast<uint8_t>(output), &trgm_output_cfg);
        return BSP_OK;
    }

    const bool has_route = is_preemption
        ? adc16_resolve_preemption_trigger_route(&trgm_base, &input, &output)
        : adc16_resolve_sequence_trigger_route(&trgm_base, &input, &output);
    if (!has_route) {
        return BSP_OK;
    }

    trgm_output_cfg.invert = false;
    trgm_output_cfg.type = trgm_output_same_as_input;
    trgm_output_cfg.input = static_cast<uint8_t>(input);
    trgm_output_config(trgm_base, static_cast<uint8_t>(output), &trgm_output_cfg);
    return BSP_OK;
}

static void fill_sequence_samples(adc16_instance_map_t *map)
{
    const auto *dma_data = reinterpret_cast<const adc16_seq_dma_data_t *>(s_adc16_seq_dma_buf[map - s_adc16_map]);
    for (uint8_t i = 0U; i < map->sequence_len; ++i) {
        map->sequence_samples[i].channel = dma_data[i].adc_ch;
        map->sequence_samples[i].index = dma_data[i].seq_num;
        map->sequence_samples[i].trigger_channel = 0xFFU;
        map->sequence_samples[i].value = static_cast<uint16_t>(dma_data[i].result);
    }
}

static void fill_preemption_samples(adc16_instance_map_t *map)
{
    const uint8_t adc_index = static_cast<uint8_t>(map - s_adc16_map);
    const auto *dma_data = reinterpret_cast<const adc16_pmt_dma_data_t *>(
        &s_adc16_pmt_dma_buf[adc_index][map->preemption_trigger_channel * k_pmt_trigger_stride]);
    for (uint8_t i = 0U; i < map->preemption_len; ++i) {
        map->preemption_samples[i].channel = dma_data[i].adc_ch;
        map->preemption_samples[i].index = dma_data[i].seq_num;
        map->preemption_samples[i].trigger_channel = dma_data[i].trig_ch;
        map->preemption_samples[i].value = static_cast<uint16_t>(dma_data[i].result);
    }
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
    bsp_adc16_init_config_t config = {};
    bsp_adc16_get_default_init_config(&config);
    return bsp_adc16_init_ex(adc, &config);
}

int32_t bsp_adc16_init_ex(bsp_adc16_instance_t adc, const bsp_adc16_init_config_t *config)
{
    if ((adc >= BSP_ADC16_MAX) || (config == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    s_adc16_map[adc].resolution = config->resolution;
    s_adc16_map[adc].clock_divider = config->clock_divider;
    const auto config_status = adc16_validate_config_features(s_adc16_map[adc], *config);
    if (config_status != BSP_OK) {
        return config_status;
    }
    const auto status = ensure_initialized(adc, adc16_conv_mode_oneshot);
    if (status != BSP_OK) {
        return status;
    }
    return adc16_configure_channel(s_adc16_map[adc].base, s_adc16_map[adc].default_channel);
}

int32_t bsp_adc16_get_clock(bsp_adc16_instance_t adc, uint32_t *freq_hz)
{
    if ((adc >= BSP_ADC16_MAX) || (freq_hz == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    if (!adc16_has_feature(s_adc16_map[adc], BSP_ADC16_FEATURE_GET_CLOCK)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (!s_adc16_map[adc].initialized || (s_adc16_map[adc].clock_hz == 0U)) {
        return BSP_ERROR;
    }
    *freq_hz = s_adc16_map[adc].clock_hz;
    return BSP_OK;
}

int32_t bsp_adc16_read_channel(bsp_adc16_instance_t adc, uint8_t channel, uint16_t *value)
{
    if ((adc >= BSP_ADC16_MAX) || (value == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    adc16_instance_map_t *map = &s_adc16_map[adc];
    if (!adc16_has_feature(*map, BSP_ADC16_FEATURE_BLOCKING_READ)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    const auto init_status = ensure_initialized(adc, adc16_conv_mode_oneshot);
    if (init_status != BSP_OK) {
        return init_status;
    }
    if (adc16_configure_channel(map->base, channel) != BSP_OK) {
        return BSP_ERROR;
    }

    for (uint32_t retry = 0U; retry < k_oneshot_retry_count; ++retry) {
        if (adc16_get_oneshot_result(map->base, channel, value) == status_success) {
            return BSP_OK;
        }
    }
    return BSP_ERROR_TIMEOUT;
}

int32_t bsp_adc16_read_default(bsp_adc16_instance_t adc, uint16_t *value)
{
    uint8_t channel;

    if (bsp_adc16_get_default_channel(adc, &channel) != BSP_OK) {
        return BSP_ERROR_PARAM;
    }

    return bsp_adc16_read_channel(adc, channel, value);
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

    auto *map = &s_adc16_map[adc];
    if (!adc16_has_feature(*map, BSP_ADC16_FEATURE_PERIOD)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    const auto init_status = ensure_initialized(adc, adc16_conv_mode_period);
    if (init_status != BSP_OK) {
        return init_status;
    }
    if (adc16_configure_channel(map->base, channel) != BSP_OK) {
        return BSP_ERROR;
    }

    adc16_prd_config_t prd_cfg = {};
    prd_cfg.ch = channel;
    prd_cfg.prescale = prescale;
    prd_cfg.period_count = period_count;
    if (adc16_set_prd_config(map->base, &prd_cfg) != status_success) {
        return BSP_ERROR;
    }

    map->period_channel = channel;
    return BSP_OK;
}

int32_t bsp_adc16_read_period(bsp_adc16_instance_t adc, uint8_t channel, uint16_t *value)
{
    if ((adc >= BSP_ADC16_MAX) || (value == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    auto *map = &s_adc16_map[adc];
    if (!adc16_has_feature(*map, BSP_ADC16_FEATURE_PERIOD)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (!map->initialized || (map->conversion_mode != adc16_conv_mode_period)) {
        return BSP_ERROR;
    }

    if (adc16_get_prd_result(map->base, channel, value) != status_success) {
        return BSP_ERROR_TIMEOUT;
    }
    return BSP_OK;
}

int32_t bsp_adc16_read_channel_async(bsp_adc16_instance_t adc, uint8_t channel)
{
    if (adc >= BSP_ADC16_MAX) {
        return BSP_ERROR_PARAM;
    }

    adc16_instance_map_t *map = &s_adc16_map[adc];
    if (!adc16_has_feature(*map, BSP_ADC16_FEATURE_INTERRUPT_READ)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (map->callback == nullptr) {
        return BSP_ERROR_PARAM;
    }

    const uint8_t single_channel = channel;
    const auto configure_status = bsp_adc16_configure_sequence(adc, &single_channel, 1U, BSP_DISABLE, BSP_DISABLE);
    if (configure_status != BSP_OK) {
        return configure_status;
    }
    map->async_channel = channel;
    map->async_started = true;
    return bsp_adc16_start_sequence(adc);
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
    s_adc16_map[adc].sequence_route = *route;
    s_adc16_map[adc].sequence_route_configured = true;
    return BSP_OK;
}

int32_t bsp_adc16_configure_sequence(bsp_adc16_instance_t adc,
                                     const uint8_t *channels,
                                     uint8_t count,
                                     bsp_state_t continuous,
                                     bsp_state_t hardware_trigger)
{
    if ((adc >= BSP_ADC16_MAX) || (channels == nullptr) || !adc16_is_valid_channel_count(count)) {
        return BSP_ERROR_PARAM;
    }

    auto *map = &s_adc16_map[adc];
    if (!adc16_has_feature(*map, BSP_ADC16_FEATURE_SEQUENCE)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if ((hardware_trigger == BSP_ENABLE) && !adc16_has_feature(*map, BSP_ADC16_FEATURE_SEQUENCE_HARDWARE_TRIGGER)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    const auto init_status = ensure_initialized(adc, adc16_conv_mode_sequence);
    if (init_status != BSP_OK) {
        return init_status;
    }

    adc16_seq_config_t seq_cfg = {};
    adc16_dma_config_t dma_cfg = {};

    for (uint8_t i = 0U; i < count; ++i) {
        const auto channel_status = adc16_configure_channel(map->base, channels[i]);
        if (channel_status != BSP_OK) {
            return channel_status;
        }
        map->sequence_channels[i] = channels[i];
        seq_cfg.queue[i].seq_int_en = (i == (count - 1U));
        seq_cfg.queue[i].ch = channels[i];
    }

    seq_cfg.seq_len = count;
    seq_cfg.restart_en = false;
    seq_cfg.cont_en = (continuous == BSP_ENABLE);
    seq_cfg.sw_trig_en = (hardware_trigger != BSP_ENABLE);
    seq_cfg.hw_trig_en = (hardware_trigger == BSP_ENABLE);
    if (adc16_set_seq_config(map->base, &seq_cfg) != status_success) {
        return BSP_ERROR;
    }

    dma_cfg.start_addr = reinterpret_cast<uint32_t *>(
        core_local_mem_to_sys_address(BOARD_RUNNING_CORE, reinterpret_cast<uint32_t>(&s_adc16_seq_dma_buf[adc][0])));
    dma_cfg.buff_len_in_4bytes = count;
    dma_cfg.stop_en = false;
    dma_cfg.stop_pos = 0U;
    if (adc16_init_seq_dma(map->base, &dma_cfg) != status_success) {
        return BSP_ERROR;
    }

    if (hardware_trigger == BSP_ENABLE) {
        const auto trigger_status = adc16_configure_trigger_route(map, false);
        if (trigger_status != BSP_OK) {
            return trigger_status;
        }
        adc16_seq_enable_hw_trigger(map->base);
    } else {
        adc16_seq_disable_hw_trigger(map->base);
    }

    adc16_enable_interrupts(map->base, adc16_event_seq_single_complete);
    intc_m_enable_irq_with_priority(map->irq, 1);
    map->async_started = false;
    map->sequence_len = count;
    map->sequence_continuous = (continuous == BSP_ENABLE);
    map->sequence_hw_trigger = (hardware_trigger == BSP_ENABLE);
    map->sequence_configured = true;
    return BSP_OK;
}

int32_t bsp_adc16_start_sequence(bsp_adc16_instance_t adc)
{
    if (adc >= BSP_ADC16_MAX) {
        return BSP_ERROR_PARAM;
    }

    auto *map = &s_adc16_map[adc];
    if (!adc16_has_feature(*map, BSP_ADC16_FEATURE_SEQUENCE)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (!map->sequence_configured) {
        return BSP_ERROR;
    }
    if (map->sequence_hw_trigger) {
        return BSP_OK;
    }
    return (adc16_trigger_seq_by_sw(map->base) == status_success) ? BSP_OK : BSP_ERROR;
}

int32_t bsp_adc16_start_sequence_dma(bsp_adc16_instance_t adc, const bsp_adc16_dma_sequence_config_t *config)
{
    if ((adc >= BSP_ADC16_MAX) || (config == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    if (!adc16_has_feature(s_adc16_map[adc], BSP_ADC16_FEATURE_SEQUENCE_DMA)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    (void) config;
    return BSP_ERROR_UNSUPPORTED;
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
    s_adc16_map[adc].preemption_route = *route;
    s_adc16_map[adc].preemption_route_configured = true;
    return BSP_OK;
}

int32_t bsp_adc16_configure_preemption(bsp_adc16_instance_t adc,
                                       uint8_t trigger_channel,
                                       const uint8_t *channels,
                                       uint8_t count,
                                       bsp_state_t hardware_trigger)
{
    if ((adc >= BSP_ADC16_MAX) || (channels == nullptr) || !adc16_is_valid_preemption_count(count)) {
        return BSP_ERROR_PARAM;
    }

    auto *map = &s_adc16_map[adc];
    if (!adc16_has_feature(*map, BSP_ADC16_FEATURE_PREEMPTION)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if ((hardware_trigger == BSP_ENABLE) && !adc16_has_feature(*map, BSP_ADC16_FEATURE_PREEMPTION_HARDWARE_TRIGGER)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    const auto init_status = ensure_initialized(adc, adc16_conv_mode_preemption);
    if (init_status != BSP_OK) {
        return init_status;
    }

    adc16_pmt_config_t pmt_cfg = {};

    for (uint8_t i = 0U; i < count; ++i) {
        const auto channel_status = adc16_configure_channel(map->base, channels[i]);
        if (channel_status != BSP_OK) {
            return channel_status;
        }
        map->preemption_channels[i] = channels[i];
        pmt_cfg.adc_ch[i] = channels[i];
        pmt_cfg.inten[i] = (i == (count - 1U));
    }

    pmt_cfg.trig_ch = trigger_channel;
    pmt_cfg.trig_len = count;
    if (adc16_set_pmt_config(map->base, &pmt_cfg) != status_success) {
        return BSP_ERROR;
    }
    if (adc16_enable_pmt_queue(map->base, trigger_channel) != status_success) {
        return BSP_ERROR;
    }

    adc16_init_pmt_dma(map->base,
        core_local_mem_to_sys_address(BOARD_RUNNING_CORE, reinterpret_cast<uint32_t>(&s_adc16_pmt_dma_buf[adc][0])));

    if (hardware_trigger == BSP_ENABLE) {
        const auto trigger_status = adc16_configure_trigger_route(map, true);
        if (trigger_status != BSP_OK) {
            return trigger_status;
        }
    }

    adc16_enable_interrupts(map->base, adc16_event_trig_complete);
    intc_m_enable_irq_with_priority(map->irq, 1);
    map->preemption_trigger_channel = trigger_channel;
    map->preemption_len = count;
    map->preemption_hw_trigger = (hardware_trigger == BSP_ENABLE);
    map->preemption_configured = true;
    return BSP_OK;
}

int32_t bsp_adc16_start_preemption(bsp_adc16_instance_t adc)
{
    if (adc >= BSP_ADC16_MAX) {
        return BSP_ERROR_PARAM;
    }

    auto *map = &s_adc16_map[adc];
    if (!adc16_has_feature(*map, BSP_ADC16_FEATURE_PREEMPTION)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (!map->preemption_configured) {
        return BSP_ERROR;
    }
    if (map->preemption_hw_trigger) {
        return BSP_OK;
    }
    return (adc16_trigger_pmt_by_sw(map->base, map->preemption_trigger_channel) == status_success) ? BSP_OK : BSP_ERROR;
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

extern "C" void bsp_adc16_irq_handler(bsp_adc16_instance_t adc)
{
    if (adc >= BSP_ADC16_MAX) {
        return;
    }

    auto &map = s_adc16_map[adc];
    const uint32_t status = adc16_get_status_flags(map.base);
    if (status == 0U) {
        return;
    }
    adc16_clear_status_flags(map.base, status);

    if ((ADC16_INT_STS_SEQ_CVC_GET(status) != 0U) && map.async_started) {
        map.async_started = false;
        if (map.callback != nullptr) {
            const auto *data = reinterpret_cast<const adc16_seq_dma_data_t *>(&s_adc16_seq_dma_buf[adc][0]);
            map.callback(data->adc_ch,
                         static_cast<uint16_t>(data->result),
                         map.callback_arg);
        }
    } else if ((ADC16_INT_STS_SEQ_CVC_GET(status) != 0U) && map.sequence_configured) {
        fill_sequence_samples(&map);
        if (map.sequence_callback != nullptr) {
            map.sequence_callback(map.sequence_samples, map.sequence_len, map.sequence_callback_arg);
        }
    }

    if ((ADC16_INT_STS_TRIG_CMPT_GET(status) != 0U) && map.preemption_configured) {
        fill_preemption_samples(&map);
        if (map.preemption_callback != nullptr) {
            map.preemption_callback(map.preemption_trigger_channel,
                                    map.preemption_samples,
                                    map.preemption_len,
                                    map.preemption_callback_arg);
        }
    }
}

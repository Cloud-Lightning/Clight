#include "bsp_adc16.h"

#include "esp_adc/adc_oneshot.h"

typedef struct {
    adc_unit_t unit;
    adc_channel_t default_channel;
    uint32_t features;
    adc_atten_t atten;
    adc_bitwidth_t bitwidth;
    bool initialized;
    adc_oneshot_unit_handle_t handle;
    bsp_adc16_callback_t callback;
    void *callback_arg;
} adc16_instance_map_t;

#define BSP_ADC16_MAP_ITEM(name, base, irq, clk, ch, clk_src_bus, has_pinmux, features) \
    { static_cast<adc_unit_t>(base), static_cast<adc_channel_t>(ch), static_cast<uint32_t>(features), static_cast<adc_atten_t>(name##_ATTEN), static_cast<adc_bitwidth_t>(name##_BITWIDTH), false, nullptr, nullptr, nullptr },
static adc16_instance_map_t s_adc16_map[BSP_ADC16_MAX] = {
    BSP_ADC16_LIST(BSP_ADC16_MAP_ITEM)
};
#undef BSP_ADC16_MAP_ITEM

static bool adc16_has_feature(const adc16_instance_map_t &map, uint32_t feature)
{
    return (map.features & feature) != 0U;
}

void bsp_adc16_get_default_init_config(bsp_adc16_init_config_t *config)
{
    if (config == nullptr) {
        return;
    }
    config->resolution = BSP_ADC16_RESOLUTION_DEFAULT;
    config->clock_divider = BSP_ADC16_CLOCK_DIVIDER_DEFAULT;
}

int32_t bsp_adc16_init(bsp_adc16_instance_t adc)
{
    if (adc >= BSP_ADC16_MAX) {
        return BSP_ERROR_PARAM;
    }

    auto *map = &s_adc16_map[adc];
    if (!adc16_has_feature(*map, BSP_ADC16_FEATURE_ONESHOT)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (map->initialized) {
        return BSP_OK;
    }

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = map->unit,
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    if (adc_oneshot_new_unit(&init_config, &map->handle) != ESP_OK) {
        return BSP_ERROR;
    }

    adc_oneshot_chan_cfg_t channel_config = {
        .atten = map->atten,
        .bitwidth = map->bitwidth,
    };
    if (adc_oneshot_config_channel(map->handle, map->default_channel, &channel_config) != ESP_OK) {
        return BSP_ERROR;
    }

    map->initialized = true;
    return BSP_OK;
}

int32_t bsp_adc16_init_ex(bsp_adc16_instance_t adc, const bsp_adc16_init_config_t *config)
{
    if ((adc >= BSP_ADC16_MAX) || (config == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    if ((config->resolution != BSP_ADC16_RESOLUTION_DEFAULT) && (config->resolution != BSP_ADC16_RESOLUTION_12_BITS)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if ((config->resolution != BSP_ADC16_RESOLUTION_DEFAULT) &&
        !adc16_has_feature(s_adc16_map[adc], BSP_ADC16_FEATURE_CONFIGURABLE_RESOLUTION)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (config->clock_divider != BSP_ADC16_CLOCK_DIVIDER_DEFAULT) {
        return BSP_ERROR_UNSUPPORTED;
    }
    return bsp_adc16_init(adc);
}

int32_t bsp_adc16_get_clock(bsp_adc16_instance_t adc, uint32_t *freq_hz)
{
    (void) adc;
    (void) freq_hz;
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_adc16_read_channel(bsp_adc16_instance_t adc, uint8_t channel, uint16_t *value)
{
    if ((adc >= BSP_ADC16_MAX) || (value == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    if (bsp_adc16_init(adc) != BSP_OK) {
        return BSP_ERROR;
    }
    if (!adc16_has_feature(s_adc16_map[adc], BSP_ADC16_FEATURE_BLOCKING_READ)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    adc_oneshot_chan_cfg_t channel_config = {
        .atten = s_adc16_map[adc].atten,
        .bitwidth = s_adc16_map[adc].bitwidth,
    };
    const auto esp_channel = static_cast<adc_channel_t>(channel);
    if (adc_oneshot_config_channel(s_adc16_map[adc].handle, esp_channel, &channel_config) != ESP_OK) {
        return BSP_ERROR_PARAM;
    }

    int raw = 0;
    if (adc_oneshot_read(s_adc16_map[adc].handle, esp_channel, &raw) != ESP_OK) {
        return BSP_ERROR_TIMEOUT;
    }

    *value = static_cast<uint16_t>(raw);
    return BSP_OK;
}

int32_t bsp_adc16_read_default(bsp_adc16_instance_t adc, uint16_t *value)
{
    if ((adc >= BSP_ADC16_MAX) || (value == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    return bsp_adc16_read_channel(adc, static_cast<uint8_t>(s_adc16_map[adc].default_channel), value);
}

int32_t bsp_adc16_get_default_channel(bsp_adc16_instance_t adc, uint8_t *channel)
{
    if ((adc >= BSP_ADC16_MAX) || (channel == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    *channel = static_cast<uint8_t>(s_adc16_map[adc].default_channel);
    return BSP_OK;
}

int32_t bsp_adc16_configure_period(bsp_adc16_instance_t adc, uint8_t channel, uint8_t prescale, uint8_t period_count)
{
    (void) adc;
    (void) channel;
    (void) prescale;
    (void) period_count;
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_adc16_read_period(bsp_adc16_instance_t adc, uint8_t channel, uint16_t *value)
{
    (void) adc;
    (void) channel;
    (void) value;
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_adc16_read_channel_async(bsp_adc16_instance_t adc, uint8_t channel)
{
    if (adc >= BSP_ADC16_MAX) {
        return BSP_ERROR_PARAM;
    }
    auto &map = s_adc16_map[adc];
    if (!adc16_has_feature(map, BSP_ADC16_FEATURE_INTERRUPT_READ)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (map.callback == nullptr) {
        return BSP_ERROR_PARAM;
    }
    uint16_t value = 0U;
    const auto status = bsp_adc16_read_channel(adc, channel, &value);
    if (status == BSP_OK) {
        map.callback(channel, value, map.callback_arg);
    }
    return status;
}

int32_t bsp_adc16_read_default_async(bsp_adc16_instance_t adc)
{
    if (adc >= BSP_ADC16_MAX) {
        return BSP_ERROR_PARAM;
    }
    return bsp_adc16_read_channel_async(adc, static_cast<uint8_t>(s_adc16_map[adc].default_channel));
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
    (void) adc;
    (void) route;
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_adc16_configure_sequence(bsp_adc16_instance_t adc,
                                     const uint8_t *channels,
                                     uint8_t count,
                                     bsp_state_t continuous,
                                     bsp_state_t hardware_trigger)
{
    (void) adc;
    (void) channels;
    (void) count;
    (void) continuous;
    (void) hardware_trigger;
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_adc16_start_sequence(bsp_adc16_instance_t adc)
{
    (void) adc;
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_adc16_register_sequence_callback(bsp_adc16_instance_t adc, bsp_adc16_sequence_callback_t callback, void *arg)
{
    (void) adc;
    (void) callback;
    (void) arg;
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_adc16_set_preemption_trigger_route(bsp_adc16_instance_t adc, const bsp_adc16_trigger_route_t *route)
{
    (void) adc;
    (void) route;
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_adc16_configure_preemption(bsp_adc16_instance_t adc,
                                       uint8_t trigger_channel,
                                       const uint8_t *channels,
                                       uint8_t count,
                                       bsp_state_t hardware_trigger)
{
    (void) adc;
    (void) trigger_channel;
    (void) channels;
    (void) count;
    (void) hardware_trigger;
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_adc16_start_preemption(bsp_adc16_instance_t adc)
{
    (void) adc;
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_adc16_register_preemption_callback(bsp_adc16_instance_t adc, bsp_adc16_preemption_callback_t callback, void *arg)
{
    (void) adc;
    (void) callback;
    (void) arg;
    return BSP_ERROR_UNSUPPORTED;
}

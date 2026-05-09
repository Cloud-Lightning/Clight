#include "bsp_pwm.h"

#include "driver/ledc.h"

typedef struct {
    ledc_mode_t speed_mode;
    ledc_timer_t timer;
    ledc_channel_t channel;
    int pin;
    ledc_timer_bit_t duty_bits;
    bool has_pinmux;
    uint32_t features;
    bool initialized;
    bool output_enabled;
    bool running;
    uint32_t frequency_hz;
    float duty_percent;
    bsp_pwm_event_callback_t callback;
    void *callback_arg;
} pwm_instance_map_t;

#define BSP_PWM_MAP_ITEM(name, backend, base, irq, clk, channel_base, has_pinmux, features) \
    { static_cast<ledc_mode_t>(name##_SPEED_MODE), static_cast<ledc_timer_t>(name##_TIMER), static_cast<ledc_channel_t>(name##_CHANNEL), static_cast<int>(name##_PIN), static_cast<ledc_timer_bit_t>(name##_DUTY_BITS), (has_pinmux) != 0U, static_cast<uint32_t>(features), false, false, false, 0U, 0.0f, nullptr, nullptr },
static pwm_instance_map_t s_pwm_map[BSP_PWM_MAX] = {
    BSP_PWM_LIST(BSP_PWM_MAP_ITEM)
};
#undef BSP_PWM_MAP_ITEM

static int32_t apply_output_state(pwm_instance_map_t *map)
{
    if ((map == nullptr) || !map->initialized) {
        return BSP_ERROR_PARAM;
    }

    if (!map->running || !map->output_enabled) {
        return (ledc_stop(map->speed_mode, map->channel, 0U) == ESP_OK) ? BSP_OK : BSP_ERROR;
    }

    const uint32_t max_duty = (1UL << static_cast<uint32_t>(map->duty_bits)) - 1UL;
    const auto duty = static_cast<uint32_t>((map->duty_percent / 100.0f) * static_cast<float>(max_duty));
    if (ledc_set_duty(map->speed_mode, map->channel, duty) != ESP_OK) {
        return BSP_ERROR;
    }
    return (ledc_update_duty(map->speed_mode, map->channel) == ESP_OK) ? BSP_OK : BSP_ERROR;
}

int32_t bsp_pwm_init(bsp_pwm_instance_t instance, uint32_t freq_hz, bsp_pwm_align_t align_mode)
{
    if ((instance >= BSP_PWM_MAX) || (freq_hz == 0U)) {
        return BSP_ERROR_PARAM;
    }
    if (align_mode != BSP_PWM_ALIGN_EDGE) {
        return BSP_ERROR_UNSUPPORTED;
    }

    auto *map = &s_pwm_map[instance];
    if (!map->has_pinmux) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if ((map->features & BSP_PWM_FEATURE_EDGE_ALIGNED) == 0U) {
        return BSP_ERROR_UNSUPPORTED;
    }

    ledc_timer_config_t timer_config = {
        .speed_mode = map->speed_mode,
        .duty_resolution = map->duty_bits,
        .timer_num = map->timer,
        .freq_hz = freq_hz,
        .clk_cfg = LEDC_AUTO_CLK,
        .deconfigure = false,
    };
    if (ledc_timer_config(&timer_config) != ESP_OK) {
        return BSP_ERROR;
    }

    ledc_channel_config_t channel_config = {
        .gpio_num = map->pin,
        .speed_mode = map->speed_mode,
        .channel = map->channel,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = map->timer,
        .duty = 0U,
        .hpoint = 0U,
        .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
        .flags = {
            .output_invert = 0,
        },
    };
    if (ledc_channel_config(&channel_config) != ESP_OK) {
        return BSP_ERROR;
    }

    map->initialized = true;
    map->output_enabled = false;
    map->running = false;
    map->frequency_hz = freq_hz;
    map->duty_percent = 0.0f;
    return apply_output_state(map);
}

int32_t bsp_pwm_get_frequency(bsp_pwm_instance_t instance, uint32_t *freq_hz)
{
    if ((instance >= BSP_PWM_MAX) || (freq_hz == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    const auto *map = &s_pwm_map[instance];
    if (!map->initialized) {
        return BSP_ERROR;
    }

    *freq_hz = map->frequency_hz;
    return BSP_OK;
}

int32_t bsp_pwm_set_duty(bsp_pwm_instance_t instance, uint8_t channel, float duty_percent)
{
    if ((instance >= BSP_PWM_MAX) || (channel != 0U)) {
        return BSP_ERROR_PARAM;
    }

    auto *map = &s_pwm_map[instance];
    if (!map->initialized) {
        return BSP_ERROR_PARAM;
    }

    if (duty_percent < 0.0f) {
        duty_percent = 0.0f;
    }
    if (duty_percent > 100.0f) {
        duty_percent = 100.0f;
    }

    map->duty_percent = duty_percent;
    const auto status = apply_output_state(map);
    if ((status == BSP_OK) && (map->callback != nullptr)) {
        map->callback(BSP_PWM_EVENT_COMPARE, map->callback_arg);
    }
    return status;
}

int32_t bsp_pwm_set_compare_ticks(bsp_pwm_instance_t instance, uint8_t compare_index, uint32_t compare_ticks)
{
    (void) instance;
    (void) compare_index;
    (void) compare_ticks;
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_pwm_enable_output(bsp_pwm_instance_t instance, uint8_t channel, bsp_state_t state)
{
    if ((instance >= BSP_PWM_MAX) || (channel != 0U)) {
        return BSP_ERROR_PARAM;
    }

    auto *map = &s_pwm_map[instance];
    if (!map->initialized) {
        return BSP_ERROR_PARAM;
    }

    map->output_enabled = (state == BSP_ENABLE);
    return apply_output_state(map);
}

int32_t bsp_pwm_enable_pair(bsp_pwm_instance_t instance, uint8_t channel, bsp_state_t state)
{
    (void) instance;
    (void) channel;
    (void) state;
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_pwm_set_deadtime(bsp_pwm_instance_t instance, uint8_t channel, uint16_t deadtime_ticks)
{
    (void) instance;
    (void) channel;
    (void) deadtime_ticks;
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_pwm_force_output(bsp_pwm_instance_t instance, uint8_t channel, bsp_level_t level)
{
    if ((instance >= BSP_PWM_MAX) || (channel != 0U)) {
        return BSP_ERROR_PARAM;
    }

    auto *map = &s_pwm_map[instance];
    if (!map->initialized) {
        return BSP_ERROR_PARAM;
    }

    map->running = true;
    map->output_enabled = true;
    map->duty_percent = (level == BSP_LEVEL_HIGH) ? 100.0f : 0.0f;
    return apply_output_state(map);
}

int32_t bsp_pwm_start(bsp_pwm_instance_t instance)
{
    if (instance >= BSP_PWM_MAX) {
        return BSP_ERROR_PARAM;
    }

    auto *map = &s_pwm_map[instance];
    if (!map->initialized) {
        return BSP_ERROR_PARAM;
    }

    map->running = true;
    const auto status = apply_output_state(map);
    if ((status == BSP_OK) && (map->callback != nullptr)) {
        map->callback(BSP_PWM_EVENT_RELOAD, map->callback_arg);
    }
    return status;
}

int32_t bsp_pwm_stop(bsp_pwm_instance_t instance)
{
    if (instance >= BSP_PWM_MAX) {
        return BSP_ERROR_PARAM;
    }

    auto *map = &s_pwm_map[instance];
    if (!map->initialized) {
        return BSP_ERROR_PARAM;
    }

    map->running = false;
    return apply_output_state(map);
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

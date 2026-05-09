#include "bsp_gptmr.h"

#include "driver/gptimer.h"

typedef struct {
    uint32_t resolution_hz;
    bool initialized;
    gptimer_handle_t handle;
    bsp_gptmr_mode_t mode;
    uint64_t period_ticks;
    bsp_gptmr_event_callback_t callback;
    void *callback_arg;
} gptmr_instance_map_t;

#define BSP_GPTMR_MAP_ITEM(name, base, channel, irq, clk, features, has_pinmux) \
    { static_cast<uint32_t>(name##_RESOLUTION_HZ), false, nullptr, BSP_GPTMR_MODE_PERIODIC, 0U, nullptr, nullptr },
static gptmr_instance_map_t s_gptmr_map[BSP_GPTMR_MAX] = {
    BSP_GPTMR_LIST(BSP_GPTMR_MAP_ITEM)
};
#undef BSP_GPTMR_MAP_ITEM

static bool gptmr_alarm_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    (void) timer;
    auto *map = static_cast<gptmr_instance_map_t *>(user_ctx);
    if ((map != nullptr) && (map->callback != nullptr) && (edata != nullptr)) {
        bsp_timer_event_t event = BSP_TIMER_EVENT_ELAPSED;
        if (map->mode == BSP_GPTMR_MODE_COMPARE) {
            event = BSP_TIMER_EVENT_COMPARE;
        } else if (map->mode == BSP_GPTMR_MODE_CAPTURE_RISING) {
            event = BSP_TIMER_EVENT_CAPTURE;
        }
        map->callback(event, static_cast<uint32_t>(edata->count_value), map->callback_arg);
    }
    return false;
}

int32_t bsp_gptmr_init(bsp_gptmr_instance_t timer, bsp_gptmr_mode_t mode, uint32_t period_ticks)
{
    if (timer >= BSP_GPTMR_MAX) {
        return BSP_ERROR_PARAM;
    }
    if (mode == BSP_GPTMR_MODE_CAPTURE_RISING) {
        return BSP_ERROR_UNSUPPORTED;
    }

    auto *map = &s_gptmr_map[timer];
    if (!map->initialized) {
        gptimer_config_t config = {
            .clk_src = GPTIMER_CLK_SRC_DEFAULT,
            .direction = GPTIMER_COUNT_UP,
            .resolution_hz = map->resolution_hz,
        };
        if (gptimer_new_timer(&config, &map->handle) != ESP_OK) {
            return BSP_ERROR;
        }
        const gptimer_event_callbacks_t callbacks = {
            .on_alarm = gptmr_alarm_callback,
        };
        if (gptimer_register_event_callbacks(map->handle, &callbacks, map) != ESP_OK) {
            return BSP_ERROR;
        }
        if (gptimer_enable(map->handle) != ESP_OK) {
            return BSP_ERROR;
        }
        map->initialized = true;
    }

    map->mode = mode;
    map->period_ticks = period_ticks;
    gptimer_set_raw_count(map->handle, 0U);
    if (period_ticks != 0U) {
        gptimer_alarm_config_t alarm = {
            .alarm_count = period_ticks,
            .reload_count = 0U,
            .flags = {
                .auto_reload_on_alarm = (mode == BSP_GPTMR_MODE_PERIODIC),
            },
        };
        if (gptimer_set_alarm_action(map->handle, &alarm) != ESP_OK) {
            return BSP_ERROR;
        }
    }
    return BSP_OK;
}

int32_t bsp_gptmr_start(bsp_gptmr_instance_t timer)
{
    if ((timer >= BSP_GPTMR_MAX) || !s_gptmr_map[timer].initialized) {
        return BSP_ERROR_PARAM;
    }
    return (gptimer_start(s_gptmr_map[timer].handle) == ESP_OK) ? BSP_OK : BSP_ERROR;
}

int32_t bsp_gptmr_stop(bsp_gptmr_instance_t timer)
{
    if ((timer >= BSP_GPTMR_MAX) || !s_gptmr_map[timer].initialized) {
        return BSP_ERROR_PARAM;
    }
    return (gptimer_stop(s_gptmr_map[timer].handle) == ESP_OK) ? BSP_OK : BSP_ERROR;
}

int32_t bsp_gptmr_get_counter(bsp_gptmr_instance_t timer, uint32_t *value)
{
    if ((timer >= BSP_GPTMR_MAX) || (value == nullptr) || !s_gptmr_map[timer].initialized) {
        return BSP_ERROR_PARAM;
    }
    uint64_t raw = 0U;
    if (gptimer_get_raw_count(s_gptmr_map[timer].handle, &raw) != ESP_OK) {
        return BSP_ERROR;
    }
    *value = static_cast<uint32_t>(raw);
    return BSP_OK;
}

int32_t bsp_gptmr_get_capture(bsp_gptmr_instance_t timer, uint32_t *value)
{
    (void) timer;
    (void) value;
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_gptmr_set_compare(bsp_gptmr_instance_t timer, uint32_t compare_ticks)
{
    if ((timer >= BSP_GPTMR_MAX) || !s_gptmr_map[timer].initialized) {
        return BSP_ERROR_PARAM;
    }
    gptimer_alarm_config_t alarm = {
        .alarm_count = compare_ticks,
        .reload_count = 0U,
        .flags = {
            .auto_reload_on_alarm = false,
        },
    };
    return (gptimer_set_alarm_action(s_gptmr_map[timer].handle, &alarm) == ESP_OK) ? BSP_OK : BSP_ERROR;
}

int32_t bsp_gptmr_get_clock(bsp_gptmr_instance_t timer, uint32_t *freq_hz)
{
    if ((timer >= BSP_GPTMR_MAX) || (freq_hz == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    *freq_hz = s_gptmr_map[timer].resolution_hz;
    return BSP_OK;
}

int32_t bsp_gptmr_register_event_callback(bsp_gptmr_instance_t timer, bsp_gptmr_event_callback_t callback, void *arg)
{
    if (timer >= BSP_GPTMR_MAX) {
        return BSP_ERROR_PARAM;
    }

    s_gptmr_map[timer].callback = callback;
    s_gptmr_map[timer].callback_arg = arg;
    return BSP_OK;
}

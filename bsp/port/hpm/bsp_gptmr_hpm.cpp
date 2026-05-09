/**
 * @file bsp_gptmr_hpm.cpp
 * @brief HPM platform implementation for BSP GPTMR interface
 */

#include "bsp_gptmr.h"
#include "board.h"

extern "C" {
#include "hpm_gptmr_drv.h"
#include "hpm_interrupt.h"
}

typedef struct {
    GPTMR_Type *base;
    uint8_t channel;
    uint32_t irq;
    uint32_t clock_name;
    uint32_t features;
    bool has_pinmux;
    bool initialized;
    uint32_t reload;
    bsp_gptmr_mode_t mode;
    bsp_gptmr_event_callback_t callback;
    void *callback_arg;
} gptmr_instance_map_t;

#define BSP_GPTMR_MAP_ITEM(name, base_addr, channel_index, irq_num, clk_name, feature_mask, has_pinmux) \
    { base_addr, channel_index, irq_num, clk_name, feature_mask, (has_pinmux) != 0U, false, 0U, BSP_GPTMR_MODE_PERIODIC, nullptr, nullptr },
static gptmr_instance_map_t s_gptmr_map[BSP_GPTMR_MAX] = {
    BSP_GPTMR_LIST(BSP_GPTMR_MAP_ITEM)
};
#undef BSP_GPTMR_MAP_ITEM

static bool bsp_gptmr_supports(const gptmr_instance_map_t *map, bsp_gptmr_mode_t mode)
{
    switch (mode) {
    case BSP_GPTMR_MODE_PERIODIC:
        return (map->features & BSP_GPTMR_FEATURE_PERIODIC) != 0U;
    case BSP_GPTMR_MODE_ONESHOT:
        return (map->features & BSP_GPTMR_FEATURE_ONESHOT) != 0U;
    case BSP_GPTMR_MODE_CAPTURE_RISING:
        return (map->features & BSP_GPTMR_FEATURE_CAPTURE) != 0U;
    case BSP_GPTMR_MODE_COMPARE:
        return (map->features & BSP_GPTMR_FEATURE_COMPARE) != 0U;
    default:
        return false;
    }
}

static void bsp_gptmr_init_channel_pin(const gptmr_instance_map_t *map, bsp_gptmr_mode_t mode)
{
    const bool as_output = (mode == BSP_GPTMR_MODE_COMPARE);

    if (map->base == HPM_GPTMR0) {
        board_init_gptmr_channel_pin(map->base, map->channel, as_output);
        return;
    }

    init_gptmr_pins(map->base);
}

int32_t bsp_gptmr_init(bsp_gptmr_instance_t timer, bsp_gptmr_mode_t mode, uint32_t period_ticks)
{
    if ((timer >= BSP_GPTMR_MAX) || (period_ticks == 0U)) {
        return BSP_ERROR_PARAM;
    }

    gptmr_instance_map_t *map = &s_gptmr_map[timer];
    gptmr_channel_config_t config;
    uint32_t freq;

    if ((!map->has_pinmux) || !bsp_gptmr_supports(map, mode)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    bsp_gptmr_init_channel_pin(map, mode);
    freq = board_init_gptmr_clock(map->base);
    if (freq == 0U) {
        return BSP_ERROR_UNSUPPORTED;
    }

    gptmr_stop_counter(map->base, map->channel);
    gptmr_channel_get_default_config(map->base, &config);
    config.reload = period_ticks;
    config.cmp[0] = (period_ticks > 1U) ? (period_ticks - 1U) : 1U;
    config.cmp[1] = period_ticks;

    switch (mode) {
    case BSP_GPTMR_MODE_PERIODIC:
        config.mode = gptmr_work_mode_no_capture;
        break;
    case BSP_GPTMR_MODE_ONESHOT:
        config.mode = gptmr_work_mode_no_capture;
#if defined(HPM_IP_FEATURE_GPTMR_OP_MODE) && (HPM_IP_FEATURE_GPTMR_OP_MODE == 1)
        config.enable_opmode = true;
#endif
        break;
    case BSP_GPTMR_MODE_CAPTURE_RISING:
        config.mode = gptmr_work_mode_capture_at_rising_edge;
        config.reload = 0xFFFFFFFFU;
        config.cmp[0] = 0U;
        config.cmp[1] = 0U;
        break;
    case BSP_GPTMR_MODE_COMPARE:
        config.mode = gptmr_work_mode_no_capture;
        config.cmp[0] = period_ticks / 2U;
        config.cmp[1] = period_ticks;
        config.enable_cmp_output = true;
        break;
    default:
        return BSP_ERROR_PARAM;
    }

    if (gptmr_channel_config(map->base, map->channel, &config, false) != status_success) {
        return BSP_ERROR;
    }

    gptmr_channel_reset_count(map->base, map->channel);
    map->initialized = true;
    map->reload = config.reload;
    map->mode = mode;
    return BSP_OK;
}

int32_t bsp_gptmr_start(bsp_gptmr_instance_t timer)
{
    if (timer >= BSP_GPTMR_MAX) {
        return BSP_ERROR_PARAM;
    }

    gptmr_instance_map_t *map = &s_gptmr_map[timer];
    if (!map->initialized) {
        return BSP_ERROR;
    }

    if (map->callback != nullptr) {
        switch (map->mode) {
        case BSP_GPTMR_MODE_PERIODIC:
        case BSP_GPTMR_MODE_ONESHOT:
            gptmr_enable_irq(map->base, GPTMR_CH_RLD_IRQ_MASK(map->channel));
            break;
        case BSP_GPTMR_MODE_CAPTURE_RISING:
            gptmr_enable_irq(map->base, GPTMR_CH_CAP_IRQ_MASK(map->channel));
            break;
        case BSP_GPTMR_MODE_COMPARE:
            gptmr_enable_irq(map->base, GPTMR_CH_CMP_IRQ_MASK(map->channel, 0U));
            break;
        default:
            break;
        }

        intc_m_enable_irq_with_priority(map->irq, 1);
    }

    gptmr_start_counter(map->base, map->channel);
    return BSP_OK;
}

int32_t bsp_gptmr_stop(bsp_gptmr_instance_t timer)
{
    if (timer >= BSP_GPTMR_MAX) {
        return BSP_ERROR_PARAM;
    }

    gptmr_stop_counter(s_gptmr_map[timer].base, s_gptmr_map[timer].channel);
    return BSP_OK;
}

int32_t bsp_gptmr_get_counter(bsp_gptmr_instance_t timer, uint32_t *value)
{
    if ((timer >= BSP_GPTMR_MAX) || (value == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    const gptmr_instance_map_t *map = &s_gptmr_map[timer];
    *value = gptmr_channel_get_counter(map->base, map->channel, gptmr_counter_type_normal);
    return BSP_OK;
}

int32_t bsp_gptmr_get_capture(bsp_gptmr_instance_t timer, uint32_t *value)
{
    if ((timer >= BSP_GPTMR_MAX) || (value == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    const gptmr_instance_map_t *map = &s_gptmr_map[timer];
    if ((map->features & BSP_GPTMR_FEATURE_CAPTURE) == 0U) {
        return BSP_ERROR_UNSUPPORTED;
    }

    *value = gptmr_channel_get_counter(map->base, map->channel, gptmr_counter_type_rising_edge);
    return BSP_OK;
}

int32_t bsp_gptmr_set_compare(bsp_gptmr_instance_t timer, uint32_t compare_ticks)
{
    if ((timer >= BSP_GPTMR_MAX) || (compare_ticks == 0U)) {
        return BSP_ERROR_PARAM;
    }

    gptmr_instance_map_t *map = &s_gptmr_map[timer];
    if ((map->features & BSP_GPTMR_FEATURE_COMPARE) == 0U) {
        return BSP_ERROR_UNSUPPORTED;
    }

    if ((map->reload != 0U) && (compare_ticks >= map->reload)) {
        compare_ticks = map->reload - 1U;
    }

    gptmr_update_cmp(map->base, map->channel, 0U, compare_ticks);
    if (map->reload != 0U) {
        gptmr_update_cmp(map->base, map->channel, 1U, map->reload);
    }
    return BSP_OK;
}

int32_t bsp_gptmr_get_clock(bsp_gptmr_instance_t timer, uint32_t *freq_hz)
{
    if ((timer >= BSP_GPTMR_MAX) || (freq_hz == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    *freq_hz = board_init_gptmr_clock(s_gptmr_map[timer].base);
    return (*freq_hz == 0U) ? BSP_ERROR_UNSUPPORTED : BSP_OK;
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

extern "C" void bsp_gptmr_irq_handler(bsp_gptmr_instance_t timer)
{
    if (timer >= BSP_GPTMR_MAX) {
        return;
    }

    auto &map = s_gptmr_map[timer];
    uint32_t value = 0U;
    switch (map.mode) {
    case BSP_GPTMR_MODE_PERIODIC:
    case BSP_GPTMR_MODE_ONESHOT:
        if (gptmr_check_status(map.base, GPTMR_CH_RLD_STAT_MASK(map.channel))) {
            gptmr_clear_status(map.base, GPTMR_CH_RLD_STAT_MASK(map.channel));
            value = map.reload;
            if (map.callback != nullptr) {
                map.callback(BSP_TIMER_EVENT_ELAPSED, value, map.callback_arg);
            }
        }
        break;
    case BSP_GPTMR_MODE_CAPTURE_RISING:
        if (gptmr_check_status(map.base, GPTMR_CH_CAP_STAT_MASK(map.channel))) {
            gptmr_clear_status(map.base, GPTMR_CH_CAP_STAT_MASK(map.channel));
            value = gptmr_channel_get_counter(map.base, map.channel, gptmr_counter_type_rising_edge);
            if (map.callback != nullptr) {
                map.callback(BSP_TIMER_EVENT_CAPTURE, value, map.callback_arg);
            }
        }
        break;
    case BSP_GPTMR_MODE_COMPARE:
        if (gptmr_check_status(map.base, GPTMR_CH_CMP_STAT_MASK(map.channel, 0U))) {
            gptmr_clear_status(map.base, GPTMR_CH_CMP_STAT_MASK(map.channel, 0U));
            value = gptmr_channel_get_counter(map.base, map.channel, gptmr_counter_type_normal);
            if (map.callback != nullptr) {
                map.callback(BSP_TIMER_EVENT_COMPARE, value, map.callback_arg);
            }
        }
        break;
    default:
        break;
    }
}

extern "C" void bsp_hpm_gptmr_timer_notify_from_board(void)
{
#if defined(BSP_GPTMR_TIMER_BASE)
    auto &map = s_gptmr_map[BSP_GPTMR_TIMER];
    if (map.callback != nullptr) {
        map.callback(BSP_TIMER_EVENT_ELAPSED, map.reload, map.callback_arg);
    }
#endif
}

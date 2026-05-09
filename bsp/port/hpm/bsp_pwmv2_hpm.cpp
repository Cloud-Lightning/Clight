/**
 * @file bsp_pwmv2_hpm.cpp
 * @brief HPM platform implementation for BSP PWMV2 interface
 */

#include "bsp_pwmv2.h"
#include "board.h"

extern "C" {
#include "hpm_clock_drv.h"
#include "hpm_interrupt.h"
#include "hpm_pwmv2_drv.h"
}

typedef struct {
    PWMV2_Type *base;
    uint32_t irq;
    clock_name_t clock_name;
    bool has_pinmux;
    bool initialized;
    uint32_t clock_hz;
    uint32_t frequency_hz;
    uint32_t reload;
    bsp_pwmv2_align_t align_mode;
    bsp_pwmv2_event_callback_t callback;
    void *callback_arg;
} pwmv2_instance_map_t;

#define BSP_PWMV2_MAP_ITEM(name, base_addr, irq_num, clk_name, has_pinmux) \
    { base_addr, irq_num, clk_name, (has_pinmux) != 0U, false, 0U, 0U, 0U, BSP_PWMV2_ALIGN_EDGE, nullptr, nullptr },
static pwmv2_instance_map_t s_pwmv2_map[BSP_PWMV2_MAX] = {
    BSP_PWMV2_LIST(BSP_PWMV2_MAP_ITEM)
};
#undef BSP_PWMV2_MAP_ITEM

namespace {

constexpr uint8_t k_waveform_shadow_stride = 3U;
constexpr uint8_t k_aux_compare_index_base = 8U;
constexpr uint8_t k_aux_compare_count = 4U;
constexpr uint8_t k_aux_shadow_index_base = 24U;

static uint8_t pwmv2_reload_shadow_index(uint8_t channel)
{
    return static_cast<uint8_t>(channel * k_waveform_shadow_stride);
}

static uint8_t pwmv2_compare_shadow_index(uint8_t channel, uint8_t compare_offset)
{
    return static_cast<uint8_t>(pwmv2_reload_shadow_index(channel) + 1U + compare_offset);
}

static bool pwmv2_is_aux_compare_index(uint8_t compare_index)
{
    return (compare_index >= k_aux_compare_index_base)
        && (compare_index < static_cast<uint8_t>(k_aux_compare_index_base + k_aux_compare_count));
}

static uint8_t pwmv2_aux_shadow_index(uint8_t compare_index)
{
    return static_cast<uint8_t>(k_aux_shadow_index_base + compare_index - k_aux_compare_index_base);
}

} // namespace

static int32_t bsp_pwmv2_apply_waveform(pwmv2_instance_map_t *map, uint8_t channel, uint32_t compare_ticks)
{
    const pwm_channel_t pwm_channel = static_cast<pwm_channel_t>(channel);
    const uint8_t cmp0 = static_cast<uint8_t>(channel * 2U);
    const uint8_t cmp1 = static_cast<uint8_t>(cmp0 + 1U);
    const uint8_t reload_shadow = pwmv2_reload_shadow_index(channel);
    const uint8_t compare_shadow0 = pwmv2_compare_shadow_index(channel, 0U);
    const uint8_t compare_shadow1 = pwmv2_compare_shadow_index(channel, 1U);

    pwmv2_shadow_register_unlock(map->base);
    if (map->align_mode == BSP_PWMV2_ALIGN_EDGE) {
        pwmv2_set_shadow_val(map->base, reload_shadow, map->reload, 0, false);
        pwmv2_set_shadow_val(map->base, compare_shadow0, compare_ticks, 0, false);
        pwmv2_set_shadow_val(map->base, compare_shadow1, map->reload + 1U, 0, false);
        pwmv2_counter_select_data_offset_from_shadow_value(map->base, pwm_counter_0, reload_shadow);
        pwmv2_counter_burst_disable(map->base, pwm_counter_0);
        pwmv2_set_reload_update_time(map->base, pwm_counter_0, pwm_reload_update_on_reload);
        pwmv2_select_cmp_source(map->base, cmp0, cmp_value_from_shadow_val, compare_shadow0);
        pwmv2_select_cmp_source(map->base, cmp1, cmp_value_from_shadow_val, compare_shadow1);
    } else {
        pwmv2_set_shadow_val(map->base, reload_shadow, map->reload, 0, false);
        pwmv2_set_shadow_val(map->base, compare_shadow0, (map->reload - compare_ticks) >> 1U, 0, false);
        pwmv2_set_shadow_val(map->base, compare_shadow1, (map->reload + compare_ticks) >> 1U, 0, false);
        pwmv2_counter_select_data_offset_from_shadow_value(map->base, pwm_counter_0, reload_shadow);
        pwmv2_counter_burst_disable(map->base, pwm_counter_0);
        pwmv2_set_reload_update_time(map->base, pwm_counter_0, pwm_reload_update_on_reload);
        pwmv2_select_cmp_source(map->base, cmp0, cmp_value_from_shadow_val, compare_shadow0);
        pwmv2_select_cmp_source(map->base, cmp1, cmp_value_from_shadow_val, compare_shadow1);
    }
    pwmv2_shadow_register_lock(map->base);
    pwmv2_disable_four_cmp(map->base, pwm_channel);
    pwmv2_channel_enable_output(map->base, pwm_channel);
    return BSP_OK;
}

int32_t bsp_pwmv2_init(bsp_pwmv2_instance_t instance, uint32_t freq_hz, bsp_pwmv2_align_t align_mode)
{
    if ((instance >= BSP_PWMV2_MAX) || (freq_hz == 0U)) {
        return BSP_ERROR_PARAM;
    }

    pwmv2_instance_map_t *map = &s_pwmv2_map[instance];
    uint32_t freq;

    if (!map->has_pinmux) {
        return BSP_ERROR_UNSUPPORTED;
    }

    init_pwm_pins(map->base);
    freq = clock_get_frequency(map->clock_name);
    if (freq == 0U) {
        clock_add_to_group(map->clock_name, BOARD_RUNNING_CORE & 0x1);
        freq = clock_get_frequency(map->clock_name);
    }
    if (freq == 0U) {
        return BSP_ERROR_UNSUPPORTED;
    }

    map->reload = freq / freq_hz;
    if (map->reload <= 1U) {
        return BSP_ERROR_PARAM;
    }

    pwmv2_deinit(map->base);
    map->clock_hz = freq;
    map->frequency_hz = freq / map->reload;
    map->align_mode = align_mode;
    (void) bsp_pwmv2_apply_waveform(map, 0U, map->reload / 2U);
    map->initialized = true;
    return BSP_OK;
}

int32_t bsp_pwmv2_get_frequency(bsp_pwmv2_instance_t instance, uint32_t *freq_hz)
{
    if ((instance >= BSP_PWMV2_MAX) || (freq_hz == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    const auto *map = &s_pwmv2_map[instance];
    if (!map->initialized || (map->frequency_hz == 0U)) {
        return BSP_ERROR;
    }

    *freq_hz = map->frequency_hz;
    return BSP_OK;
}

int32_t bsp_pwmv2_set_duty(bsp_pwmv2_instance_t instance, uint8_t channel, float duty_percent)
{
    if ((instance >= BSP_PWMV2_MAX) || (channel > 5U) || (duty_percent < 0.0f) || (duty_percent > 100.0f)) {
        return BSP_ERROR_PARAM;
    }

    pwmv2_instance_map_t *map = &s_pwmv2_map[instance];
    uint32_t compare_ticks;

    if (!map->initialized || (map->reload == 0U)) {
        return BSP_ERROR;
    }

    compare_ticks = static_cast<uint32_t>((duty_percent * static_cast<float>(map->reload)) / 100.0f);
    if (compare_ticks >= map->reload) {
        compare_ticks = map->reload - 1U;
    }

    return bsp_pwmv2_apply_waveform(map, channel, compare_ticks);
}

int32_t bsp_pwmv2_set_compare_ticks(bsp_pwmv2_instance_t instance, uint8_t compare_index, uint32_t compare_ticks)
{
    if (instance >= BSP_PWMV2_MAX) {
        return BSP_ERROR_PARAM;
    }

    auto *map = &s_pwmv2_map[instance];
    if (!map->initialized || (map->reload == 0U)) {
        return BSP_ERROR;
    }
    if (!pwmv2_is_aux_compare_index(compare_index)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    if (compare_ticks >= map->reload) {
        compare_ticks = map->reload - 1U;
    }

    pwmv2_cmp_config_t compare_config = {};
    compare_config.cmp = compare_ticks;
    compare_config.enable_half_cmp = false;
    compare_config.enable_hrcmp = false;
    compare_config.cmp_source = cmp_value_from_shadow_val;
    compare_config.cmp_source_index = pwmv2_aux_shadow_index(compare_index);
    compare_config.mode = 0U;
    compare_config.update_trigger = pwm_shadow_register_update_on_reload;
    compare_config.update_trigger_index = 0U;
    compare_config.hrcmp = 0U;
    pwmv2_config_cmp(map->base, compare_index, &compare_config);
    return BSP_OK;
}

int32_t bsp_pwmv2_enable_output(bsp_pwmv2_instance_t instance, uint8_t channel, bsp_state_t state)
{
    if ((instance >= BSP_PWMV2_MAX) || (channel > 5U)) {
        return BSP_ERROR_PARAM;
    }

    PWMV2_Type *base = s_pwmv2_map[instance].base;
    pwm_channel_t pwm_channel = static_cast<pwm_channel_t>(channel);

    if (state == BSP_ENABLE) {
        pwmv2_channel_enable_output(base, pwm_channel);
    } else {
        pwmv2_channel_disable_output(base, pwm_channel);
    }
    return BSP_OK;
}

int32_t bsp_pwmv2_enable_pair(bsp_pwmv2_instance_t instance, uint8_t channel, bsp_state_t state)
{
    if ((instance >= BSP_PWMV2_MAX) || (channel > 5U)) {
        return BSP_ERROR_PARAM;
    }

    PWMV2_Type *base = s_pwmv2_map[instance].base;
    pwm_channel_t pwm_channel = static_cast<pwm_channel_t>(channel);
    if (state == BSP_ENABLE) {
        pwmv2_enable_pair_mode(base, pwm_channel);
    } else {
        pwmv2_disable_pair_mode(base, pwm_channel);
    }
    return BSP_OK;
}

int32_t bsp_pwmv2_set_deadtime(bsp_pwmv2_instance_t instance, uint8_t channel, uint16_t deadtime_ticks)
{
    if ((instance >= BSP_PWMV2_MAX) || (channel > 5U)) {
        return BSP_ERROR_PARAM;
    }

    pwmv2_set_dead_area(s_pwmv2_map[instance].base, static_cast<pwm_channel_t>(channel), deadtime_ticks);
    return BSP_OK;
}

int32_t bsp_pwmv2_force_output(bsp_pwmv2_instance_t instance, uint8_t channel, bsp_level_t level)
{
    if ((instance >= BSP_PWMV2_MAX) || (channel > 5U)) {
        return BSP_ERROR_PARAM;
    }

    PWMV2_Type *base = s_pwmv2_map[instance].base;
    pwm_channel_t pwm_channel = static_cast<pwm_channel_t>(channel);

    pwmv2_enable_shadow_lock_feature(base);
    pwmv2_set_force_update_time(base, pwm_channel, pwm_force_immediately);
    pwmv2_enable_force_by_software(base, pwm_channel);
    pwmv2_enable_software_force(base, pwm_channel);
    pwmv2_shadow_register_unlock(base);
    pwmv2_force_output(base, pwm_channel, (level == BSP_LEVEL_HIGH) ? pwm_force_output_1 : pwm_force_output_0, false);
    pwmv2_shadow_register_lock(base);
    return BSP_OK;
}

int32_t bsp_pwmv2_start(bsp_pwmv2_instance_t instance)
{
    if (instance >= BSP_PWMV2_MAX) {
        return BSP_ERROR_PARAM;
    }

    auto *map = &s_pwmv2_map[instance];
    if (map->callback != nullptr) {
        pwmv2_enable_reload_irq(map->base, pwm_counter_0);
        for (uint8_t compare_index = 0U; compare_index < PWM_SOC_CMP_MAX_COUNT; ++compare_index) {
            pwmv2_enable_cmp_irq(map->base, compare_index);
        }
        pwmv2_enable_fault_irq(map->base, pwm_channel_0);
        intc_m_enable_irq_with_priority(map->irq, 1);
    }
    pwmv2_enable_counter(map->base, pwm_counter_0);
    pwmv2_start_pwm_output(map->base, pwm_counter_0);
    return BSP_OK;
}

int32_t bsp_pwmv2_stop(bsp_pwmv2_instance_t instance)
{
    if (instance >= BSP_PWMV2_MAX) {
        return BSP_ERROR_PARAM;
    }

    auto *map = &s_pwmv2_map[instance];
    pwmv2_disable_counter(map->base, pwm_counter_0);
    pwmv2_disable_reload_irq(map->base, pwm_counter_0);
    for (uint8_t compare_index = 0U; compare_index < PWM_SOC_CMP_MAX_COUNT; ++compare_index) {
        pwmv2_disable_cmp_irq(map->base, compare_index);
    }
    pwmv2_disable_fault_irq(map->base, pwm_channel_0);
    return BSP_OK;
}

int32_t bsp_pwmv2_register_event_callback(bsp_pwmv2_instance_t instance, bsp_pwmv2_event_callback_t callback, void *arg)
{
    if (instance >= BSP_PWMV2_MAX) {
        return BSP_ERROR_PARAM;
    }

    s_pwmv2_map[instance].callback = callback;
    s_pwmv2_map[instance].callback_arg = arg;
    return BSP_OK;
}

extern "C" void bsp_pwmv2_irq_handler(bsp_pwmv2_instance_t instance)
{
    if (instance >= BSP_PWMV2_MAX) {
        return;
    }

    auto &map = s_pwmv2_map[instance];
    uint32_t event_mask = 0U;

    const uint32_t reload_status = pwmv2_get_reload_irq_status(map.base);
    if (reload_status != 0U) {
        pwmv2_clear_reload_irq_status(map.base, reload_status);
        event_mask |= BSP_PWMV2_EVENT_RELOAD;
    }

    const uint32_t cmp_status = pwmv2_get_cmp_irq_status(map.base);
    if (cmp_status != 0U) {
        pwmv2_clear_cmp_irq_status(map.base, cmp_status);
        event_mask |= BSP_PWMV2_EVENT_COMPARE;
    }

    const uint32_t fault_status = pwmv2_get_fault_irq_status(map.base);
    if (fault_status != 0U) {
        pwmv2_clear_fault_irq_status(map.base, fault_status);
        event_mask |= BSP_PWMV2_EVENT_FAULT;
    }

    if ((event_mask != 0U) && (map.callback != nullptr)) {
        map.callback(event_mask, map.callback_arg);
    }
}

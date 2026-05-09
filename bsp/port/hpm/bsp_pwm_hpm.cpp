/**
 * @file bsp_pwm_hpm.cpp
 * @brief HPM platform implementation for generic BSP PWM capability
 */

#include "bsp_pwm.h"
#include "board.h"

extern "C" {
#include "hpm_clock_drv.h"
#include "hpm_gptmr_drv.h"
#include "hpm_interrupt.h"
#include "hpm_pwmv2_drv.h"
}

namespace {

constexpr uint8_t k_pwmv2_channel_count = 6U;
constexpr uint8_t k_gptmr_channel_count = 2U;
constexpr uint8_t k_waveform_shadow_stride = 3U;
constexpr uint8_t k_aux_compare_index_base = 8U;
constexpr uint8_t k_aux_compare_count = 4U;
constexpr uint8_t k_aux_shadow_index_base = 24U;

typedef struct {
    uint32_t backend;
    void *base;
    uint32_t irq;
    clock_name_t clock_name;
    uint8_t channel_base;
    bool has_pinmux;
    uint32_t features;
    bool initialized;
    uint32_t clock_hz;
    uint32_t frequency_hz;
    uint32_t reload;
    bsp_pwm_align_t align_mode;
    bsp_pwm_event_callback_t callback;
    void *callback_arg;
} pwm_instance_map_t;

#define BSP_PWM_MAP_ITEM(name, backend, base_addr, irq_num, clk_name, channel_base, has_pinmux, features) \
    { backend, base_addr, irq_num, clk_name, static_cast<uint8_t>(channel_base), (has_pinmux) != 0U, static_cast<uint32_t>(features), false, 0U, 0U, 0U, BSP_PWM_ALIGN_EDGE, nullptr, nullptr },
static pwm_instance_map_t s_pwm_map[BSP_PWM_MAX] = {
    BSP_PWM_LIST(BSP_PWM_MAP_ITEM)
};
#undef BSP_PWM_MAP_ITEM

static uint8_t physical_channel(const pwm_instance_map_t &map, uint8_t logical_channel)
{
    return static_cast<uint8_t>(map.channel_base + logical_channel);
}

static bool pwm_supports(const pwm_instance_map_t &map, uint32_t feature)
{
    return (map.features & feature) != 0U;
}

static bool is_valid_channel(const pwm_instance_map_t &map, uint8_t logical_channel)
{
    const uint8_t channel = physical_channel(map, logical_channel);
    if (map.backend == BSP_PWM_BACKEND_HPM_GPTMR) {
        return channel < k_gptmr_channel_count;
    }
    if (map.backend == BSP_PWM_BACKEND_HPM_PWMV2) {
        return channel < k_pwmv2_channel_count;
    }
    return false;
}

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

static PWMV2_Type *pwmv2_base(const pwm_instance_map_t &map)
{
    return static_cast<PWMV2_Type *>(map.base);
}

static GPTMR_Type *gptmr_base(const pwm_instance_map_t &map)
{
    return static_cast<GPTMR_Type *>(map.base);
}

static int32_t enable_pwm_clock(pwm_instance_map_t &map)
{
    uint32_t freq = clock_get_frequency(map.clock_name);
    if (freq == 0U) {
        clock_add_to_group(map.clock_name, BOARD_RUNNING_CORE & 0x1U);
        freq = clock_get_frequency(map.clock_name);
    }
    if (freq == 0U) {
        return BSP_ERROR_UNSUPPORTED;
    }

    map.clock_hz = freq;
    return BSP_OK;
}

static int32_t apply_pwmv2_waveform(pwm_instance_map_t &map, uint8_t logical_channel, uint32_t compare_ticks)
{
    if (!is_valid_channel(map, logical_channel) || (map.reload == 0U)) {
        return BSP_ERROR_PARAM;
    }

    const uint8_t channel = physical_channel(map, logical_channel);
    const pwm_channel_t pwm_channel = static_cast<pwm_channel_t>(channel);
    const uint8_t cmp0 = static_cast<uint8_t>(channel * 2U);
    const uint8_t cmp1 = static_cast<uint8_t>(cmp0 + 1U);
    const uint8_t reload_shadow = pwmv2_reload_shadow_index(channel);
    const uint8_t compare_shadow0 = pwmv2_compare_shadow_index(channel, 0U);
    const uint8_t compare_shadow1 = pwmv2_compare_shadow_index(channel, 1U);
    PWMV2_Type *base = pwmv2_base(map);

    if (compare_ticks >= map.reload) {
        compare_ticks = map.reload - 1U;
    }

    pwmv2_shadow_register_unlock(base);
    if (map.align_mode == BSP_PWM_ALIGN_EDGE) {
        pwmv2_set_shadow_val(base, reload_shadow, map.reload, 0, false);
        pwmv2_set_shadow_val(base, compare_shadow0, compare_ticks, 0, false);
        pwmv2_set_shadow_val(base, compare_shadow1, map.reload + 1U, 0, false);
    } else {
        pwmv2_set_shadow_val(base, reload_shadow, map.reload, 0, false);
        pwmv2_set_shadow_val(base, compare_shadow0, (map.reload - compare_ticks) >> 1U, 0, false);
        pwmv2_set_shadow_val(base, compare_shadow1, (map.reload + compare_ticks) >> 1U, 0, false);
    }

    pwmv2_counter_select_data_offset_from_shadow_value(base, pwm_counter_0, reload_shadow);
    pwmv2_counter_burst_disable(base, pwm_counter_0);
    pwmv2_set_reload_update_time(base, pwm_counter_0, pwm_reload_update_on_reload);
    pwmv2_select_cmp_source(base, cmp0, cmp_value_from_shadow_val, compare_shadow0);
    pwmv2_select_cmp_source(base, cmp1, cmp_value_from_shadow_val, compare_shadow1);
    pwmv2_shadow_register_lock(base);
    pwmv2_disable_four_cmp(base, pwm_channel);
    return BSP_OK;
}

static int32_t configure_gptmr_channel(pwm_instance_map_t &map, uint8_t logical_channel, uint32_t compare_ticks)
{
    if (!is_valid_channel(map, logical_channel) || (map.reload <= 1U)) {
        return BSP_ERROR_PARAM;
    }

    const uint8_t channel = physical_channel(map, logical_channel);
    GPTMR_Type *base = gptmr_base(map);
    if (compare_ticks >= map.reload) {
        compare_ticks = map.reload - 1U;
    }

    gptmr_channel_config_t cfg;
    gptmr_channel_get_default_config(base, &cfg);
    cfg.reload = map.reload;
    cfg.cmp_initial_polarity_high = false;

    gptmr_stop_counter(base, channel);
    gptmr_channel_config(base, channel, &cfg, false);
    gptmr_channel_reset_count(base, channel);
    gptmr_update_cmp(base, channel, 0, compare_ticks);
    gptmr_update_cmp(base, channel, 1, map.reload);
    return BSP_OK;
}

static int32_t ensure_instance_ready(bsp_pwm_instance_t instance)
{
    if (instance >= BSP_PWM_MAX) {
        return BSP_ERROR_PARAM;
    }

    auto &map = s_pwm_map[instance];
    if (!map.has_pinmux) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if ((map.backend != BSP_PWM_BACKEND_HPM_GPTMR) && (map.backend != BSP_PWM_BACKEND_HPM_PWMV2)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    return BSP_OK;
}

} // namespace

int32_t bsp_pwm_init(bsp_pwm_instance_t instance, uint32_t freq_hz, bsp_pwm_align_t align_mode)
{
    if ((instance >= BSP_PWM_MAX) || (freq_hz == 0U)) {
        return BSP_ERROR_PARAM;
    }

    const auto ready_status = ensure_instance_ready(instance);
    if (ready_status != BSP_OK) {
        return ready_status;
    }

    auto &map = s_pwm_map[instance];
    if ((align_mode == BSP_PWM_ALIGN_EDGE) && !pwm_supports(map, BSP_PWM_FEATURE_EDGE_ALIGNED)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if ((align_mode == BSP_PWM_ALIGN_CENTER) && !pwm_supports(map, BSP_PWM_FEATURE_CENTER_ALIGNED)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    const auto clock_status = enable_pwm_clock(map);
    if (clock_status != BSP_OK) {
        return clock_status;
    }

    map.reload = map.clock_hz / freq_hz;
    if (map.reload <= 1U) {
        return BSP_ERROR_PARAM;
    }

    map.frequency_hz = map.clock_hz / map.reload;
    map.align_mode = align_mode;

    if (map.backend == BSP_PWM_BACKEND_HPM_PWMV2) {
        PWMV2_Type *base = pwmv2_base(map);
        init_pwm_pins(base);
        pwmv2_deinit(base);
        const auto status = apply_pwmv2_waveform(map, 0U, 0U);
        if (status != BSP_OK) {
            return status;
        }
    } else {
        init_gptmr_pins(gptmr_base(map));
        const auto status = configure_gptmr_channel(map, 0U, 0U);
        if (status != BSP_OK) {
            return status;
        }
    }

    map.initialized = true;
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

    auto &map = s_pwm_map[instance];
    if (!map.initialized || (map.reload == 0U) || !is_valid_channel(map, channel)) {
        return BSP_ERROR_PARAM;
    }

    if (duty_percent < 0.0f) {
        duty_percent = 0.0f;
    }
    if (duty_percent > 100.0f) {
        duty_percent = 100.0f;
    }

    uint32_t compare_ticks = static_cast<uint32_t>((duty_percent * static_cast<float>(map.reload)) / 100.0f);
    if (compare_ticks >= map.reload) {
        compare_ticks = map.reload - 1U;
    }

    if (map.backend == BSP_PWM_BACKEND_HPM_PWMV2) {
        return apply_pwmv2_waveform(map, channel, compare_ticks);
    }
    return configure_gptmr_channel(map, channel, compare_ticks);
}

int32_t bsp_pwm_set_compare_ticks(bsp_pwm_instance_t instance, uint8_t compare_index, uint32_t compare_ticks)
{
    if (instance >= BSP_PWM_MAX) {
        return BSP_ERROR_PARAM;
    }

    auto &map = s_pwm_map[instance];
    if (!map.initialized || (map.reload == 0U)) {
        return BSP_ERROR;
    }

    if (map.backend == BSP_PWM_BACKEND_HPM_GPTMR) {
        return (compare_index == 0U) ? configure_gptmr_channel(map, 0U, compare_ticks) : BSP_ERROR_UNSUPPORTED;
    }

    if (!pwm_supports(map, BSP_PWM_FEATURE_TRIGGER_COMPARE) || !pwmv2_is_aux_compare_index(compare_index)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (compare_ticks >= map.reload) {
        compare_ticks = map.reload - 1U;
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
    pwmv2_config_cmp(pwmv2_base(map), compare_index, &compare_config);
    return BSP_OK;
}

int32_t bsp_pwm_enable_output(bsp_pwm_instance_t instance, uint8_t channel, bsp_state_t state)
{
    if (instance >= BSP_PWM_MAX) {
        return BSP_ERROR_PARAM;
    }

    auto &map = s_pwm_map[instance];
    if (!is_valid_channel(map, channel)) {
        return BSP_ERROR_PARAM;
    }

    if (map.backend == BSP_PWM_BACKEND_HPM_GPTMR) {
        if (state == BSP_DISABLE) {
            return configure_gptmr_channel(map, channel, 0U);
        }
        return BSP_OK;
    }

    PWMV2_Type *base = pwmv2_base(map);
    const pwm_channel_t pwm_channel = static_cast<pwm_channel_t>(physical_channel(map, channel));
    if (state == BSP_ENABLE) {
        pwmv2_channel_enable_output(base, pwm_channel);
    } else {
        pwmv2_channel_disable_output(base, pwm_channel);
    }
    return BSP_OK;
}

int32_t bsp_pwm_enable_pair(bsp_pwm_instance_t instance, uint8_t channel, bsp_state_t state)
{
    if (instance >= BSP_PWM_MAX) {
        return BSP_ERROR_PARAM;
    }

    auto &map = s_pwm_map[instance];
    if (!is_valid_channel(map, channel)) {
        return BSP_ERROR_PARAM;
    }
    if ((map.backend != BSP_PWM_BACKEND_HPM_PWMV2) || !pwm_supports(map, BSP_PWM_FEATURE_COMPLEMENTARY)) {
        return (state == BSP_DISABLE) ? BSP_OK : BSP_ERROR_UNSUPPORTED;
    }

    PWMV2_Type *base = pwmv2_base(map);
    const pwm_channel_t pwm_channel = static_cast<pwm_channel_t>(physical_channel(map, channel));
    if (state == BSP_ENABLE) {
        pwmv2_enable_pair_mode(base, pwm_channel);
    } else {
        pwmv2_disable_pair_mode(base, pwm_channel);
    }
    return BSP_OK;
}

int32_t bsp_pwm_set_deadtime(bsp_pwm_instance_t instance, uint8_t channel, uint16_t deadtime_ticks)
{
    if (instance >= BSP_PWM_MAX) {
        return BSP_ERROR_PARAM;
    }

    auto &map = s_pwm_map[instance];
    if (!is_valid_channel(map, channel)) {
        return BSP_ERROR_PARAM;
    }
    if ((map.backend != BSP_PWM_BACKEND_HPM_PWMV2) || !pwm_supports(map, BSP_PWM_FEATURE_DEADTIME)) {
        return (deadtime_ticks == 0U) ? BSP_OK : BSP_ERROR_UNSUPPORTED;
    }

    pwmv2_set_dead_area(pwmv2_base(map), static_cast<pwm_channel_t>(physical_channel(map, channel)), deadtime_ticks);
    return BSP_OK;
}

int32_t bsp_pwm_force_output(bsp_pwm_instance_t instance, uint8_t channel, bsp_level_t level)
{
    if (instance >= BSP_PWM_MAX) {
        return BSP_ERROR_PARAM;
    }

    auto &map = s_pwm_map[instance];
    if (!is_valid_channel(map, channel)) {
        return BSP_ERROR_PARAM;
    }
    if (map.backend == BSP_PWM_BACKEND_HPM_GPTMR) {
        return bsp_pwm_set_duty(instance, channel, (level == BSP_LEVEL_HIGH) ? 100.0f : 0.0f);
    }

    PWMV2_Type *base = pwmv2_base(map);
    const pwm_channel_t pwm_channel = static_cast<pwm_channel_t>(physical_channel(map, channel));
    pwmv2_enable_shadow_lock_feature(base);
    pwmv2_set_force_update_time(base, pwm_channel, pwm_force_immediately);
    pwmv2_enable_force_by_software(base, pwm_channel);
    pwmv2_enable_software_force(base, pwm_channel);
    pwmv2_shadow_register_unlock(base);
    pwmv2_force_output(base, pwm_channel, (level == BSP_LEVEL_HIGH) ? pwm_force_output_1 : pwm_force_output_0, false);
    pwmv2_shadow_register_lock(base);
    return BSP_OK;
}

int32_t bsp_pwm_start(bsp_pwm_instance_t instance)
{
    if (instance >= BSP_PWM_MAX) {
        return BSP_ERROR_PARAM;
    }

    auto &map = s_pwm_map[instance];
    if (!map.initialized) {
        return BSP_ERROR;
    }

    if (map.backend == BSP_PWM_BACKEND_HPM_GPTMR) {
        gptmr_start_counter(gptmr_base(map), map.channel_base);
        return BSP_OK;
    }

    PWMV2_Type *base = pwmv2_base(map);
    if (map.callback != nullptr) {
        pwmv2_enable_reload_irq(base, pwm_counter_0);
        for (uint8_t compare_index = 0U; compare_index < PWM_SOC_CMP_MAX_COUNT; ++compare_index) {
            pwmv2_enable_cmp_irq(base, compare_index);
        }
        pwmv2_enable_fault_irq(base, pwm_channel_0);
        intc_m_enable_irq_with_priority(map.irq, 1);
    }
    pwmv2_enable_counter(base, pwm_counter_0);
    pwmv2_start_pwm_output(base, pwm_counter_0);
    return BSP_OK;
}

int32_t bsp_pwm_stop(bsp_pwm_instance_t instance)
{
    if (instance >= BSP_PWM_MAX) {
        return BSP_ERROR_PARAM;
    }

    auto &map = s_pwm_map[instance];
    if (!map.initialized) {
        return BSP_ERROR;
    }

    if (map.backend == BSP_PWM_BACKEND_HPM_GPTMR) {
        gptmr_stop_counter(gptmr_base(map), map.channel_base);
        return BSP_OK;
    }

    PWMV2_Type *base = pwmv2_base(map);
    pwmv2_disable_counter(base, pwm_counter_0);
    pwmv2_disable_reload_irq(base, pwm_counter_0);
    for (uint8_t compare_index = 0U; compare_index < PWM_SOC_CMP_MAX_COUNT; ++compare_index) {
        pwmv2_disable_cmp_irq(base, compare_index);
    }
    pwmv2_disable_fault_irq(base, pwm_channel_0);
    return BSP_OK;
}

int32_t bsp_pwm_register_event_callback(bsp_pwm_instance_t instance, bsp_pwm_event_callback_t callback, void *arg)
{
    if (instance >= BSP_PWM_MAX) {
        return BSP_ERROR_PARAM;
    }

    auto &map = s_pwm_map[instance];
    if ((callback != nullptr) && (map.backend != BSP_PWM_BACKEND_HPM_PWMV2)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    map.callback = callback;
    map.callback_arg = arg;
    return BSP_OK;
}

extern "C" void bsp_pwm_irq_handler(bsp_pwm_instance_t instance)
{
    if (instance >= BSP_PWM_MAX) {
        return;
    }

    auto &map = s_pwm_map[instance];
    if ((map.backend != BSP_PWM_BACKEND_HPM_PWMV2) || (map.callback == nullptr)) {
        return;
    }

    uint32_t event_mask = 0U;
    PWMV2_Type *base = pwmv2_base(map);

    const uint32_t reload_status = pwmv2_get_reload_irq_status(base);
    if (reload_status != 0U) {
        pwmv2_clear_reload_irq_status(base, reload_status);
        event_mask |= BSP_PWM_EVENT_RELOAD;
    }

    const uint32_t cmp_status = pwmv2_get_cmp_irq_status(base);
    if (cmp_status != 0U) {
        pwmv2_clear_cmp_irq_status(base, cmp_status);
        event_mask |= BSP_PWM_EVENT_COMPARE;
    }

    const uint32_t fault_status = pwmv2_get_fault_irq_status(base);
    if (fault_status != 0U) {
        pwmv2_clear_fault_irq_status(base, fault_status);
        event_mask |= BSP_PWM_EVENT_FAULT;
    }

    if (event_mask != 0U) {
        map.callback(event_mask, map.callback_arg);
    }
}

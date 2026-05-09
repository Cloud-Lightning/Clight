/**
 * @file bsp_pwmv2.h
 * @brief BSP PWMV2 interface
 */

#ifndef BSP_PWMV2_H
#define BSP_PWMV2_H

#include "bsp_common.h"
#include "bsp_pwmv2_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_PWMV2_LIST
#define BSP_PWMV2_LIST(X) \
    X(BSP_PWMV2_MAIN, BSP_PWMV2_MAIN_BASE, BSP_PWMV2_MAIN_IRQ, BSP_PWMV2_MAIN_CLK_NAME, BSP_PWMV2_MAIN_HAS_PINMUX)
#endif

typedef enum {
#define BSP_PWMV2_ENUM_ITEM(name, base, irq, clk, has_pinmux) name,
    BSP_PWMV2_LIST(BSP_PWMV2_ENUM_ITEM)
#undef BSP_PWMV2_ENUM_ITEM
    BSP_PWMV2_MAX
} bsp_pwmv2_instance_t;

typedef enum {
    BSP_PWMV2_ALIGN_EDGE = 0,
    BSP_PWMV2_ALIGN_CENTER = 1
} bsp_pwmv2_align_t;

typedef enum {
    BSP_PWMV2_TRIGGER_OUTPUT2_DISABLE = 0,
    BSP_PWMV2_TRIGGER_OUTPUT2_UPDATE,
    BSP_PWMV2_TRIGGER_OUTPUT2_OC1,
    BSP_PWMV2_TRIGGER_OUTPUT2_OC1REF,
    BSP_PWMV2_TRIGGER_OUTPUT2_OC2REF,
    BSP_PWMV2_TRIGGER_OUTPUT2_OC3REF,
    BSP_PWMV2_TRIGGER_OUTPUT2_OC4REF,
    BSP_PWMV2_TRIGGER_OUTPUT2_OC4REF_RISINGFALLING,
    BSP_PWMV2_TRIGGER_OUTPUT2_OC5REF,
    BSP_PWMV2_TRIGGER_OUTPUT2_OC6REF
} bsp_pwmv2_trigger_output2_t;

typedef bsp_pwmv2_event_callback_t bsp_pwmv2_callback_t;

int32_t bsp_pwmv2_init(bsp_pwmv2_instance_t instance, uint32_t freq_hz, bsp_pwmv2_align_t align_mode);
int32_t bsp_pwmv2_get_frequency(bsp_pwmv2_instance_t instance, uint32_t *freq_hz);
int32_t bsp_pwmv2_set_duty(bsp_pwmv2_instance_t instance, uint8_t channel, float duty_percent);
int32_t bsp_pwmv2_set_compare_ticks(bsp_pwmv2_instance_t instance, uint8_t compare_index, uint32_t compare_ticks);
int32_t bsp_pwmv2_config_trigger_output2(bsp_pwmv2_instance_t instance, bsp_pwmv2_trigger_output2_t output);
int32_t bsp_pwmv2_enable_output(bsp_pwmv2_instance_t instance, uint8_t channel, bsp_state_t state);
int32_t bsp_pwmv2_enable_pair(bsp_pwmv2_instance_t instance, uint8_t channel, bsp_state_t state);
int32_t bsp_pwmv2_set_deadtime(bsp_pwmv2_instance_t instance, uint8_t channel, uint16_t deadtime_ticks);
int32_t bsp_pwmv2_force_output(bsp_pwmv2_instance_t instance, uint8_t channel, bsp_level_t level);
int32_t bsp_pwmv2_start(bsp_pwmv2_instance_t instance);
int32_t bsp_pwmv2_stop(bsp_pwmv2_instance_t instance);
int32_t bsp_pwmv2_register_event_callback(bsp_pwmv2_instance_t instance, bsp_pwmv2_event_callback_t callback, void *arg);

static inline int32_t bsp_pwmv2_register_callback(bsp_pwmv2_instance_t instance, bsp_pwmv2_callback_t callback, void *arg)
{
    return bsp_pwmv2_register_event_callback(instance, callback, arg);
}

#ifdef __cplusplus
}
#endif

#endif /* BSP_PWMV2_H */

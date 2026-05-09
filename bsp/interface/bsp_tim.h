/**
 * @file bsp_tim.h
 * @brief BSP TIM interface
 */

#ifndef BSP_TIM_H
#define BSP_TIM_H

#include "bsp_common.h"
#include "bsp_timer_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_TIM_FEATURE_PERIODIC
#define BSP_TIM_FEATURE_PERIODIC   (1U << 0)
#define BSP_TIM_FEATURE_ONESHOT    (1U << 1)
#define BSP_TIM_FEATURE_CAPTURE    (1U << 2)
#define BSP_TIM_FEATURE_COMPARE    (1U << 3)
#define BSP_TIM_FEATURE_TRIGGER_OUTPUT (1U << 4)
#endif

#ifndef BSP_TIM_LIST
#define BSP_TIM_LIST(X) \
    X(BSP_TIM_MAIN, BSP_TIM_MAIN_BASE, BSP_TIM_MAIN_IRQ, BSP_TIM_MAIN_FEATURES, BSP_TIM_MAIN_HAS_PINMUX)
#endif

typedef enum {
#define BSP_TIM_ENUM_ITEM(name, base, irq, features, has_pinmux) name,
    BSP_TIM_LIST(BSP_TIM_ENUM_ITEM)
#undef BSP_TIM_ENUM_ITEM
    BSP_TIM_MAX
} bsp_tim_instance_t;

typedef enum {
    BSP_TIM_MODE_PERIODIC = 0,
    BSP_TIM_MODE_ONESHOT,
    BSP_TIM_MODE_CAPTURE_RISING,
    BSP_TIM_MODE_COMPARE
} bsp_tim_mode_t;

typedef enum {
    BSP_TIM_TRIGGER_OUTPUT_DISABLE = 0,
    BSP_TIM_TRIGGER_OUTPUT_UPDATE,
    BSP_TIM_TRIGGER_OUTPUT_OC1,
    BSP_TIM_TRIGGER_OUTPUT_OC2,
    BSP_TIM_TRIGGER_OUTPUT_OC3,
    BSP_TIM_TRIGGER_OUTPUT_OC4
} bsp_tim_trigger_output_t;

typedef bsp_timer_event_t bsp_tim_event_t;
typedef bsp_timer_event_callback_t bsp_tim_event_callback_t;
typedef bsp_tim_event_callback_t bsp_tim_callback_t;

int32_t bsp_tim_init(bsp_tim_instance_t timer, bsp_tim_mode_t mode, uint32_t period_ticks);
int32_t bsp_tim_start(bsp_tim_instance_t timer);
int32_t bsp_tim_stop(bsp_tim_instance_t timer);
int32_t bsp_tim_get_counter(bsp_tim_instance_t timer, uint32_t *value);
int32_t bsp_tim_get_capture(bsp_tim_instance_t timer, uint32_t *value);
int32_t bsp_tim_set_compare(bsp_tim_instance_t timer, uint32_t compare_ticks);
int32_t bsp_tim_get_clock(bsp_tim_instance_t timer, uint32_t *freq_hz);
int32_t bsp_tim_config_trigger_output(bsp_tim_instance_t timer, bsp_tim_trigger_output_t output);
int32_t bsp_tim_register_event_callback(bsp_tim_instance_t timer, bsp_tim_event_callback_t callback, void *arg);

static inline int32_t bsp_tim_register_callback(bsp_tim_instance_t timer, bsp_tim_callback_t callback, void *arg)
{
    return bsp_tim_register_event_callback(timer, callback, arg);
}

#ifdef __cplusplus
}
#endif

#endif /* BSP_TIM_H */

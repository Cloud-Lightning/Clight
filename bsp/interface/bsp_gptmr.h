/**
 * @file bsp_gptmr.h
 * @brief BSP GPTMR interface
 */

#ifndef BSP_GPTMR_H
#define BSP_GPTMR_H

#include "bsp_common.h"
#include "bsp_timer_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_GPTMR_LIST
#define BSP_GPTMR_LIST(X) \
    X(BSP_GPTMR_TIMER, BSP_GPTMR_TIMER_BASE, BSP_GPTMR_TIMER_CHANNEL, BSP_GPTMR_TIMER_IRQ, BSP_GPTMR_TIMER_CLK_NAME, BSP_GPTMR_FEATURE_PERIODIC | BSP_GPTMR_FEATURE_ONESHOT, 1U)
#endif

typedef enum {
#define BSP_GPTMR_ENUM_ITEM(name, base, channel, irq, clk, features, has_pinmux) name,
    BSP_GPTMR_LIST(BSP_GPTMR_ENUM_ITEM)
#undef BSP_GPTMR_ENUM_ITEM
    BSP_GPTMR_MAX
} bsp_gptmr_instance_t;

typedef enum {
    BSP_GPTMR_MODE_PERIODIC = 0,
    BSP_GPTMR_MODE_ONESHOT,
    BSP_GPTMR_MODE_CAPTURE_RISING,
    BSP_GPTMR_MODE_COMPARE
} bsp_gptmr_mode_t;

typedef bsp_timer_event_t bsp_gptmr_event_t;
typedef bsp_timer_event_callback_t bsp_gptmr_event_callback_t;
typedef bsp_gptmr_event_callback_t bsp_gptmr_callback_t;

int32_t bsp_gptmr_init(bsp_gptmr_instance_t timer, bsp_gptmr_mode_t mode, uint32_t period_ticks);
int32_t bsp_gptmr_start(bsp_gptmr_instance_t timer);
int32_t bsp_gptmr_stop(bsp_gptmr_instance_t timer);
int32_t bsp_gptmr_get_counter(bsp_gptmr_instance_t timer, uint32_t *value);
int32_t bsp_gptmr_get_capture(bsp_gptmr_instance_t timer, uint32_t *value);
int32_t bsp_gptmr_set_compare(bsp_gptmr_instance_t timer, uint32_t compare_ticks);
int32_t bsp_gptmr_get_clock(bsp_gptmr_instance_t timer, uint32_t *freq_hz);
int32_t bsp_gptmr_register_event_callback(bsp_gptmr_instance_t timer, bsp_gptmr_event_callback_t callback, void *arg);

static inline int32_t bsp_gptmr_register_callback(bsp_gptmr_instance_t timer, bsp_gptmr_callback_t callback, void *arg)
{
    return bsp_gptmr_register_event_callback(timer, callback, arg);
}

#ifdef __cplusplus
}
#endif

#endif /* BSP_GPTMR_H */

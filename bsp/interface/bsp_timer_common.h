/**
 * @file bsp_timer_common.h
 * @brief Common timer event types shared by BSP timer backends
 */

#ifndef BSP_TIMER_COMMON_H
#define BSP_TIMER_COMMON_H

#include "bsp_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BSP_TIMER_EVENT_ELAPSED = 0,
    BSP_TIMER_EVENT_CAPTURE,
    BSP_TIMER_EVENT_COMPARE,
} bsp_timer_event_t;

typedef void (*bsp_timer_event_callback_t)(bsp_timer_event_t event, uint32_t value, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* BSP_TIMER_COMMON_H */

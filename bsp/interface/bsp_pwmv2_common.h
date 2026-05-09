/**
 * @file bsp_pwmv2_common.h
 * @brief Common PWMV2 event types shared by BSP PWMV2 backends
 */

#ifndef BSP_PWMV2_COMMON_H
#define BSP_PWMV2_COMMON_H

#include "bsp_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BSP_PWMV2_EVENT_RELOAD = 1U << 0,
    BSP_PWMV2_EVENT_COMPARE = 1U << 1,
    BSP_PWMV2_EVENT_FAULT = 1U << 2
} bsp_pwmv2_event_t;

typedef void (*bsp_pwmv2_event_callback_t)(uint32_t event_mask, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* BSP_PWMV2_COMMON_H */

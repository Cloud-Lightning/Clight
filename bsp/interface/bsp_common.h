/**
 * @file bsp_common.h
 * @brief BSP 通用定义和类型
 */

#ifndef BSP_COMMON_H
#define BSP_COMMON_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* BSP 返回值定义 */
#define BSP_OK              (0)
#define BSP_ERROR           (-1)
#define BSP_ERROR_PARAM     (-2)
#define BSP_ERROR_TIMEOUT   (-3)
#define BSP_ERROR_BUSY      (-4)
#define BSP_ERROR_UNSUPPORTED (-5)
#define BSP_IRQ_PRIORITY_DEFAULT (1U)

/* 电平定义 */
typedef enum {
    BSP_LEVEL_LOW = 0,
    BSP_LEVEL_HIGH = 1
} bsp_level_t;

/* 使能/禁用 */
typedef enum {
    BSP_DISABLE = 0,
    BSP_ENABLE = 1
} bsp_state_t;

typedef enum {
    BSP_TRANSFER_MODE_AUTO = 0,
    BSP_TRANSFER_MODE_BLOCKING = 1,
    BSP_TRANSFER_MODE_INTERRUPT = 2,
    BSP_TRANSFER_MODE_DMA = 3
} bsp_transfer_mode_t;

#ifdef __cplusplus
}
#endif

#endif /* BSP_COMMON_H */

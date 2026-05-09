/**
 * @file bsp_qeiv2.h
 * @brief BSP QEIV2 interface
 */

#ifndef BSP_QEIV2_H
#define BSP_QEIV2_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_QEIV2_LIST
#define BSP_QEIV2_LIST(X) \
    X(BSP_QEIV2_MAIN, BSP_QEIV2_MAIN_BASE, BSP_QEIV2_MAIN_IRQ, BSP_QEIV2_MAIN_CLK_NAME, BSP_QEIV2_MAIN_PHASE_COUNT_PER_REV, BSP_QEIV2_MAIN_HAS_PINMUX)
#endif

typedef enum {
#define BSP_QEIV2_ENUM_ITEM(name, base, irq, clk, phase_count, has_pinmux) name,
    BSP_QEIV2_LIST(BSP_QEIV2_ENUM_ITEM)
#undef BSP_QEIV2_ENUM_ITEM
    BSP_QEIV2_MAX
} bsp_qeiv2_instance_t;

int32_t bsp_qeiv2_init(bsp_qeiv2_instance_t qei);
int32_t bsp_qeiv2_get_position(bsp_qeiv2_instance_t qei, uint32_t *position);
int32_t bsp_qeiv2_get_angle(bsp_qeiv2_instance_t qei, uint32_t *angle);
int32_t bsp_qeiv2_get_phase_count(bsp_qeiv2_instance_t qei, uint32_t *phase_count);
int32_t bsp_qeiv2_get_speed(bsp_qeiv2_instance_t qei, uint32_t *speed);

#ifdef __cplusplus
}
#endif

#endif /* BSP_QEIV2_H */

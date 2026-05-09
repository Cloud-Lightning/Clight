/**
 * @file bsp_qeov2.h
 * @brief BSP QEOV2 interface
 */

#ifndef BSP_QEOV2_H
#define BSP_QEOV2_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_QEOV2_LIST
#define BSP_QEOV2_LIST(X) \
    X(BSP_QEOV2_MAIN, BSP_QEOV2_MAIN_BASE, BSP_QEOV2_MAIN_CLK_NAME, BSP_QEOV2_MAIN_HAS_PINMUX)
#endif

typedef enum {
#define BSP_QEOV2_ENUM_ITEM(name, base, clk, has_pinmux) name,
    BSP_QEOV2_LIST(BSP_QEOV2_ENUM_ITEM)
#undef BSP_QEOV2_ENUM_ITEM
    BSP_QEOV2_MAX
} bsp_qeov2_instance_t;

int32_t bsp_qeov2_init(bsp_qeov2_instance_t instance);
int32_t bsp_qeov2_start(bsp_qeov2_instance_t instance);
int32_t bsp_qeov2_stop(bsp_qeov2_instance_t instance);

#ifdef __cplusplus
}
#endif

#endif /* BSP_QEOV2_H */

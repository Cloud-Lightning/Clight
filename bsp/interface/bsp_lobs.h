/**
 * @file bsp_lobs.h
 * @brief BSP LOBS interface
 */

#ifndef BSP_LOBS_H
#define BSP_LOBS_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_LOBS_LIST
#define BSP_LOBS_LIST(X) \
    X(BSP_LOBS_MAIN, BSP_LOBS_MAIN_BASE, BSP_LOBS_MAIN_CLK_NAME)
#endif

typedef enum {
#define BSP_LOBS_ENUM_ITEM(name, base, clk) name,
    BSP_LOBS_LIST(BSP_LOBS_ENUM_ITEM)
#undef BSP_LOBS_ENUM_ITEM
    BSP_LOBS_MAX
} bsp_lobs_instance_t;

int32_t bsp_lobs_init(bsp_lobs_instance_t instance);
int32_t bsp_lobs_enable(bsp_lobs_instance_t instance, bsp_state_t state);

#ifdef __cplusplus
}
#endif

#endif /* BSP_LOBS_H */

/**
 * @file bsp_mchtmr.h
 * @brief BSP MCHTMR interface
 */

#ifndef BSP_MCHTMR_H
#define BSP_MCHTMR_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_MCHTMR_LIST
#define BSP_MCHTMR_LIST(X) \
    X(BSP_MCHTMR_MAIN, BSP_MCHTMR_MAIN_BASE, BSP_MCHTMR_MAIN_CLK_NAME)
#endif

typedef enum {
#define BSP_MCHTMR_ENUM_ITEM(name, base, clk) name,
    BSP_MCHTMR_LIST(BSP_MCHTMR_ENUM_ITEM)
#undef BSP_MCHTMR_ENUM_ITEM
    BSP_MCHTMR_MAX
} bsp_mchtmr_instance_t;

int32_t bsp_mchtmr_get_count(bsp_mchtmr_instance_t instance, uint64_t *count);
int32_t bsp_mchtmr_delay_ticks(bsp_mchtmr_instance_t instance, uint64_t delay_ticks);

#ifdef __cplusplus
}
#endif

#endif /* BSP_MCHTMR_H */

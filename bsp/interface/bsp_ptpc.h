/**
 * @file bsp_ptpc.h
 * @brief BSP PTPC interface
 */

#ifndef BSP_PTPC_H
#define BSP_PTPC_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_PTPC_LIST
#define BSP_PTPC_LIST(X) \
    X(BSP_PTPC_MAIN, BSP_PTPC_MAIN_BASE, BSP_PTPC_MAIN_IRQ, BSP_PTPC_MAIN_CLK_NAME)
#endif

typedef enum {
#define BSP_PTPC_ENUM_ITEM(name, base, irq, clk) name,
    BSP_PTPC_LIST(BSP_PTPC_ENUM_ITEM)
#undef BSP_PTPC_ENUM_ITEM
    BSP_PTPC_MAX
} bsp_ptpc_instance_t;

int32_t bsp_ptpc_init(bsp_ptpc_instance_t instance, uint8_t timer_index);
int32_t bsp_ptpc_get_time(bsp_ptpc_instance_t instance, uint8_t timer_index, uint32_t *sec_out, uint32_t *nsec_out);

#ifdef __cplusplus
}
#endif

#endif /* BSP_PTPC_H */

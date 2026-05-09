/**
 * @file bsp_plb.h
 * @brief BSP PLB interface
 */

#ifndef BSP_PLB_H
#define BSP_PLB_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_PLB_LIST
#define BSP_PLB_LIST(X) \
    X(BSP_PLB_MAIN, BSP_PLB_MAIN_BASE, BSP_PLB_MAIN_CLK_NAME)
#endif

typedef enum {
#define BSP_PLB_ENUM_ITEM(name, base, clk) name,
    BSP_PLB_LIST(BSP_PLB_ENUM_ITEM)
#undef BSP_PLB_ENUM_ITEM
    BSP_PLB_MAX
} bsp_plb_instance_t;

int32_t bsp_plb_init(bsp_plb_instance_t instance);
int32_t bsp_plb_get_counter(bsp_plb_instance_t instance, uint8_t channel, uint32_t *counter_out);

#ifdef __cplusplus
}
#endif

#endif /* BSP_PLB_H */

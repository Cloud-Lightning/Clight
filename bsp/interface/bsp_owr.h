/**
 * @file bsp_owr.h
 * @brief BSP OWR interface
 */

#ifndef BSP_OWR_H
#define BSP_OWR_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_OWR_LIST
#define BSP_OWR_LIST(X) \
    X(BSP_OWR_MAIN, BSP_OWR_MAIN_BASE, BSP_OWR_MAIN_CLK_NAME, BSP_OWR_MAIN_HAS_PINMUX)
#endif

typedef enum {
#define BSP_OWR_ENUM_ITEM(name, base, clk, has_pinmux) name,
    BSP_OWR_LIST(BSP_OWR_ENUM_ITEM)
#undef BSP_OWR_ENUM_ITEM
    BSP_OWR_MAX
} bsp_owr_instance_t;

int32_t bsp_owr_init(bsp_owr_instance_t instance);
int32_t bsp_owr_get_presence(bsp_owr_instance_t instance, uint32_t *status_out);

#ifdef __cplusplus
}
#endif

#endif /* BSP_OWR_H */

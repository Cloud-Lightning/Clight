/**
 * @file bsp_tsns.h
 * @brief BSP TSNS interface
 */

#ifndef BSP_TSNS_H
#define BSP_TSNS_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_TSNS_LIST
#define BSP_TSNS_LIST(X) \
    X(BSP_TSNS_MAIN, BSP_TSNS_MAIN_BASE, BSP_TSNS_MAIN_CLK_NAME)
#endif

typedef enum {
#define BSP_TSNS_ENUM_ITEM(name, base, clk) name,
    BSP_TSNS_LIST(BSP_TSNS_ENUM_ITEM)
#undef BSP_TSNS_ENUM_ITEM
    BSP_TSNS_MAX
} bsp_tsns_instance_t;

int32_t bsp_tsns_init(bsp_tsns_instance_t instance);
int32_t bsp_tsns_read_celsius(bsp_tsns_instance_t instance, float *temperature);

#ifdef __cplusplus
}
#endif

#endif /* BSP_TSNS_H */

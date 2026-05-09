/**
 * @file bsp_acmp.h
 * @brief BSP ACMP interface
 */

#ifndef BSP_ACMP_H
#define BSP_ACMP_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_ACMP_LIST
#define BSP_ACMP_LIST(X) \
    X(BSP_ACMP_MAIN, BSP_ACMP_MAIN_BASE, BSP_ACMP_MAIN_CLK_NAME, BSP_ACMP_MAIN_HAS_PINMUX)
#endif

typedef enum {
#define BSP_ACMP_ENUM_ITEM(name, base, clk, has_pinmux) name,
    BSP_ACMP_LIST(BSP_ACMP_ENUM_ITEM)
#undef BSP_ACMP_ENUM_ITEM
    BSP_ACMP_MAX
} bsp_acmp_instance_t;

int32_t bsp_acmp_init(bsp_acmp_instance_t instance, uint8_t channel);
int32_t bsp_acmp_get_status(bsp_acmp_instance_t instance, uint8_t channel, uint32_t *status_out);

#ifdef __cplusplus
}
#endif

#endif /* BSP_ACMP_H */

/**
 * @file bsp_eui.h
 * @brief BSP EUI interface
 */

#ifndef BSP_EUI_H
#define BSP_EUI_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_EUI_LIST
#define BSP_EUI_LIST(X) \
    X(BSP_EUI_MAIN, BSP_EUI_MAIN_BASE, BSP_EUI_MAIN_CLK_NAME, BSP_EUI_MAIN_HAS_PINMUX)
#endif

typedef enum {
#define BSP_EUI_ENUM_ITEM(name, base, clk, has_pinmux) name,
    BSP_EUI_LIST(BSP_EUI_ENUM_ITEM)
#undef BSP_EUI_ENUM_ITEM
    BSP_EUI_MAX
} bsp_eui_instance_t;

int32_t bsp_eui_init(bsp_eui_instance_t instance);
int32_t bsp_eui_enable(bsp_eui_instance_t instance, bsp_state_t state);
int32_t bsp_eui_get_irq_status(bsp_eui_instance_t instance, uint32_t *status_out);

#ifdef __cplusplus
}
#endif

#endif /* BSP_EUI_H */

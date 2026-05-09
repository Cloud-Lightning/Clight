#ifndef BSP_HCD_H
#define BSP_HCD_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_HCD_LIST
#define BSP_HCD_LIST(X) \
    X(BSP_HCD_HS, BSP_HCD_HS_BASE, BSP_HCD_HS_HAS_PINMUX)
#endif

typedef enum {
#define BSP_HCD_ENUM_ITEM(name, base, available) name,
    BSP_HCD_LIST(BSP_HCD_ENUM_ITEM)
#undef BSP_HCD_ENUM_ITEM
    BSP_HCD_MAX
} bsp_hcd_instance_t;

int32_t bsp_hcd_init(bsp_hcd_instance_t instance);

#ifdef __cplusplus
}
#endif

#endif

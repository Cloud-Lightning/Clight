#ifndef BSP_PCD_H
#define BSP_PCD_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_PCD_LIST
#define BSP_PCD_LIST(X) \
    X(BSP_PCD_HS, BSP_PCD_HS_BASE, BSP_PCD_HS_HAS_PINMUX)
#endif

typedef enum {
#define BSP_PCD_ENUM_ITEM(name, base, available) name,
    BSP_PCD_LIST(BSP_PCD_ENUM_ITEM)
#undef BSP_PCD_ENUM_ITEM
    BSP_PCD_MAX
} bsp_pcd_instance_t;

int32_t bsp_pcd_init(bsp_pcd_instance_t instance);

#ifdef __cplusplus
}
#endif

#endif

#ifndef BSP_DFSDM_H
#define BSP_DFSDM_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_DFSDM_LIST
#define BSP_DFSDM_LIST(X) \
    X(BSP_DFSDM1, BSP_DFSDM1_BASE, BSP_DFSDM1_HAS_PINMUX)
#endif

typedef enum {
#define BSP_DFSDM_ENUM_ITEM(name, base, available) name,
    BSP_DFSDM_LIST(BSP_DFSDM_ENUM_ITEM)
#undef BSP_DFSDM_ENUM_ITEM
    BSP_DFSDM_MAX
} bsp_dfsdm_instance_t;

int32_t bsp_dfsdm_init(bsp_dfsdm_instance_t instance);

#ifdef __cplusplus
}
#endif

#endif

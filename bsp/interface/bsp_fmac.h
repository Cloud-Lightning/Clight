#ifndef BSP_FMAC_H
#define BSP_FMAC_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_FMAC_LIST
#define BSP_FMAC_LIST(X) \
    X(BSP_FMAC_MAIN, BSP_FMAC_MAIN_BASE)
#endif

typedef enum {
#define BSP_FMAC_ENUM_ITEM(name, base) name,
    BSP_FMAC_LIST(BSP_FMAC_ENUM_ITEM)
#undef BSP_FMAC_ENUM_ITEM
    BSP_FMAC_MAX
} bsp_fmac_instance_t;

int32_t bsp_fmac_init(bsp_fmac_instance_t instance);

#ifdef __cplusplus
}
#endif

#endif

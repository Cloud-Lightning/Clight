#ifndef BSP_PPA_H
#define BSP_PPA_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_PPA_LIST
#define BSP_PPA_LIST(X) \
    X(BSP_PPA0, BSP_PPA0_BASE, BSP_PPA0_AVAILABLE)
#endif

typedef enum {
#define BSP_PPA_ENUM_ITEM(name, base, available) name,
    BSP_PPA_LIST(BSP_PPA_ENUM_ITEM)
#undef BSP_PPA_ENUM_ITEM
    BSP_PPA_MAX
} bsp_ppa_instance_t;

int32_t bsp_ppa_init(bsp_ppa_instance_t instance);

#ifdef __cplusplus
}
#endif

#endif

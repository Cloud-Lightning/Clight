#ifndef BSP_PARLIO_H
#define BSP_PARLIO_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_PARLIO_LIST
#define BSP_PARLIO_LIST(X) \
    X(BSP_PARLIO0, BSP_PARLIO0_BASE, BSP_PARLIO0_HAS_PINMUX)
#endif

typedef enum {
#define BSP_PARLIO_ENUM_ITEM(name, base, available) name,
    BSP_PARLIO_LIST(BSP_PARLIO_ENUM_ITEM)
#undef BSP_PARLIO_ENUM_ITEM
    BSP_PARLIO_MAX
} bsp_parlio_instance_t;

int32_t bsp_parlio_init(bsp_parlio_instance_t instance);

#ifdef __cplusplus
}
#endif

#endif

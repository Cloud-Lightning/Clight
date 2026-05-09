#ifndef BSP_COMP_H
#define BSP_COMP_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_COMP_LIST
#define BSP_COMP_LIST(X) \
    X(BSP_COMP1, BSP_COMP1_BASE, BSP_COMP1_HAS_PINMUX)
#endif

typedef enum {
#define BSP_COMP_ENUM_ITEM(name, base, available) name,
    BSP_COMP_LIST(BSP_COMP_ENUM_ITEM)
#undef BSP_COMP_ENUM_ITEM
    BSP_COMP_MAX
} bsp_comp_instance_t;

int32_t bsp_comp_init(bsp_comp_instance_t instance);

#ifdef __cplusplus
}
#endif

#endif

#ifndef BSP_OPAMP_H
#define BSP_OPAMP_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_OPAMP_LIST
#define BSP_OPAMP_LIST(X) \
    X(BSP_OPAMP1, BSP_OPAMP1_BASE, BSP_OPAMP1_HAS_PINMUX)
#endif

typedef enum {
#define BSP_OPAMP_ENUM_ITEM(name, base, available) name,
    BSP_OPAMP_LIST(BSP_OPAMP_ENUM_ITEM)
#undef BSP_OPAMP_ENUM_ITEM
    BSP_OPAMP_MAX
} bsp_opamp_instance_t;

int32_t bsp_opamp_init(bsp_opamp_instance_t instance);

#ifdef __cplusplus
}
#endif

#endif

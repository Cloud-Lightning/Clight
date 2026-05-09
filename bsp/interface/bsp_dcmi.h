#ifndef BSP_DCMI_H
#define BSP_DCMI_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_DCMI_LIST
#define BSP_DCMI_LIST(X) \
    X(BSP_DCMI, BSP_DCMI_BASE, BSP_DCMI_HAS_PINMUX)
#endif

typedef enum {
#define BSP_DCMI_ENUM_ITEM(name, base, available) name,
    BSP_DCMI_LIST(BSP_DCMI_ENUM_ITEM)
#undef BSP_DCMI_ENUM_ITEM
    BSP_DCMI_MAX
} bsp_dcmi_instance_t;

int32_t bsp_dcmi_init(bsp_dcmi_instance_t instance);

#ifdef __cplusplus
}
#endif

#endif

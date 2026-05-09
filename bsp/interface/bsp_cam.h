#ifndef BSP_CAM_H
#define BSP_CAM_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_CAM_LIST
#define BSP_CAM_LIST(X) \
    X(BSP_CAM0, BSP_CAM0_BASE, BSP_CAM0_HAS_PINMUX)
#endif

typedef enum {
#define BSP_CAM_ENUM_ITEM(name, base, available) name,
    BSP_CAM_LIST(BSP_CAM_ENUM_ITEM)
#undef BSP_CAM_ENUM_ITEM
    BSP_CAM_MAX
} bsp_cam_instance_t;

int32_t bsp_cam_init(bsp_cam_instance_t instance);

#ifdef __cplusplus
}
#endif

#endif

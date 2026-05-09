#ifndef BSP_JPEG_H
#define BSP_JPEG_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_JPEG_LIST
#define BSP_JPEG_LIST(X) \
    X(BSP_JPEG, BSP_JPEG_BASE, BSP_JPEG_AVAILABLE)
#endif

typedef enum {
#define BSP_JPEG_ENUM_ITEM(name, base, available) name,
    BSP_JPEG_LIST(BSP_JPEG_ENUM_ITEM)
#undef BSP_JPEG_ENUM_ITEM
    BSP_JPEG_MAX
} bsp_jpeg_instance_t;

int32_t bsp_jpeg_init(bsp_jpeg_instance_t instance);

#ifdef __cplusplus
}
#endif

#endif

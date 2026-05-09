#ifndef BSP_DMA2D_H
#define BSP_DMA2D_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_DMA2D_LIST
#define BSP_DMA2D_LIST(X) \
    X(BSP_DMA2D_MAIN, BSP_DMA2D_MAIN_BASE)
#endif

typedef enum {
#define BSP_DMA2D_ENUM_ITEM(name, base) name,
    BSP_DMA2D_LIST(BSP_DMA2D_ENUM_ITEM)
#undef BSP_DMA2D_ENUM_ITEM
    BSP_DMA2D_MAX
} bsp_dma2d_instance_t;

int32_t bsp_dma2d_init(bsp_dma2d_instance_t instance);

#ifdef __cplusplus
}
#endif

#endif

#include "bsp_dma2d.h"

typedef struct {
    DMA2D_TypeDef *base;
    bool initialized;
} dma2d_map_t;

#define BSP_DMA2D_MAP_ITEM(name, base) { base, false },
static dma2d_map_t s_dma2d_map[BSP_DMA2D_MAX] = {
    BSP_DMA2D_LIST(BSP_DMA2D_MAP_ITEM)
};
#undef BSP_DMA2D_MAP_ITEM

int32_t bsp_dma2d_init(bsp_dma2d_instance_t instance)
{
    if (instance >= BSP_DMA2D_MAX) {
        return BSP_ERROR_PARAM;
    }

    __HAL_RCC_DMA2D_CLK_ENABLE();
    s_dma2d_map[instance].initialized = true;
    return BSP_OK;
}

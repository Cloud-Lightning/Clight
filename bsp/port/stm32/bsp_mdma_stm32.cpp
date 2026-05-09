#include "bsp_mdma.h"

typedef struct {
    MDMA_TypeDef *base;
    bool initialized;
} mdma_map_t;

#define BSP_MDMA_MAP_ITEM(name, base) { base, false },
static mdma_map_t s_mdma_map[BSP_MDMA_MAX] = {
    BSP_MDMA_LIST(BSP_MDMA_MAP_ITEM)
};
#undef BSP_MDMA_MAP_ITEM

int32_t bsp_mdma_init(bsp_mdma_instance_t instance)
{
    if (instance >= BSP_MDMA_MAX) {
        return BSP_ERROR_PARAM;
    }

    __HAL_RCC_MDMA_CLK_ENABLE();
    s_mdma_map[instance].initialized = true;
    return BSP_OK;
}

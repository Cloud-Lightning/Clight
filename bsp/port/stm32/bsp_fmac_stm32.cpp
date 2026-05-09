#include "bsp_fmac.h"

typedef struct {
    FMAC_TypeDef *base;
    bool initialized;
} fmac_map_t;

#define BSP_FMAC_MAP_ITEM(name, base) { base, false },
static fmac_map_t s_fmac_map[BSP_FMAC_MAX] = {
    BSP_FMAC_LIST(BSP_FMAC_MAP_ITEM)
};
#undef BSP_FMAC_MAP_ITEM

int32_t bsp_fmac_init(bsp_fmac_instance_t instance)
{
    if (instance >= BSP_FMAC_MAX) {
        return BSP_ERROR_PARAM;
    }

    __HAL_RCC_FMAC_CLK_ENABLE();
    s_fmac_map[instance].initialized = true;
    return BSP_OK;
}

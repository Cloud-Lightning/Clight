#include "bsp_cordic.h"

typedef struct {
    CORDIC_TypeDef *base;
    bool initialized;
} cordic_map_t;

#define BSP_CORDIC_MAP_ITEM(name, base) { base, false },
static cordic_map_t s_cordic_map[BSP_CORDIC_MAX] = {
    BSP_CORDIC_LIST(BSP_CORDIC_MAP_ITEM)
};
#undef BSP_CORDIC_MAP_ITEM

int32_t bsp_cordic_init(bsp_cordic_instance_t instance)
{
    if (instance >= BSP_CORDIC_MAX) {
        return BSP_ERROR_PARAM;
    }

    __HAL_RCC_CORDIC_CLK_ENABLE();
    s_cordic_map[instance].initialized = true;
    return BSP_OK;
}

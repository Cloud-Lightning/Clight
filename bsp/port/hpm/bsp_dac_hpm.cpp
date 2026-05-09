#include "bsp_dac.h"

typedef struct {
    uintptr_t base;
    bool has_pinmux;
    uint32_t features;
} dac_map_t;

#define BSP_DAC_MAP_ITEM(name, base, has_pinmux, features) { (uintptr_t)(base), (has_pinmux) != 0U, features },
static const dac_map_t s_dac_map[BSP_DAC_MAX] = {
    BSP_DAC_LIST(BSP_DAC_MAP_ITEM)
};
#undef BSP_DAC_MAP_ITEM

static int32_t dac_require(bsp_dac_instance_t instance)
{
    if (instance >= BSP_DAC_MAX) {
        return BSP_ERROR_PARAM;
    }
    const dac_map_t *map = &s_dac_map[instance];
    return ((map->base != 0U) && map->has_pinmux && ((map->features & BSP_DAC_FEATURE_DIRECT_WRITE) != 0U)) ? BSP_OK : BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_dac_init(bsp_dac_instance_t instance)
{
    return dac_require(instance);
}

int32_t bsp_dac_write(bsp_dac_instance_t instance, uint8_t channel, uint32_t value)
{
    (void) value;
    if ((channel == 0U) || (channel > 2U)) {
        return BSP_ERROR_PARAM;
    }
    return dac_require(instance);
}

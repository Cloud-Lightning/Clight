#include "bsp_rng.h"

typedef struct {
    uintptr_t base;
    uint32_t features;
} rng_map_t;

#define BSP_RNG_MAP_ITEM(name, base, features) { (uintptr_t)(base), features },
static const rng_map_t s_rng_map[BSP_RNG_MAX] = {
    BSP_RNG_LIST(BSP_RNG_MAP_ITEM)
};
#undef BSP_RNG_MAP_ITEM

int32_t bsp_rng_init(bsp_rng_instance_t instance)
{
    if (instance >= BSP_RNG_MAX) {
        return BSP_ERROR_PARAM;
    }
    return ((s_rng_map[instance].base != 0U) && ((s_rng_map[instance].features & BSP_RNG_FEATURE_U32) != 0U)) ? BSP_OK : BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_rng_get_u32(bsp_rng_instance_t instance, uint32_t *value)
{
    if ((instance >= BSP_RNG_MAX) || (value == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR_UNSUPPORTED;
}

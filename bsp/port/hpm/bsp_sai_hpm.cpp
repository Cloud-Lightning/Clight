#include "bsp_sai.h"

typedef struct {
    uintptr_t base;
    bool has_pinmux;
    uint32_t features;
} sai_map_t;

#define BSP_SAI_MAP_ITEM(name, base, has_pinmux, features) { (uintptr_t)(base), (has_pinmux) != 0U, features },
static const sai_map_t s_sai_map[BSP_SAI_MAX] = {
    BSP_SAI_LIST(BSP_SAI_MAP_ITEM)
};
#undef BSP_SAI_MAP_ITEM

static int32_t sai_require(bsp_sai_instance_t instance, uint32_t feature)
{
    if (instance >= BSP_SAI_MAX) {
        return BSP_ERROR_PARAM;
    }
    const sai_map_t *map = &s_sai_map[instance];
    return ((map->base != 0U) && map->has_pinmux && ((map->features & feature) != 0U)) ? BSP_OK : BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_sai_init(bsp_sai_instance_t instance)
{
    return sai_require(instance, BSP_SAI_FEATURE_INIT);
}

int32_t bsp_sai_write(bsp_sai_instance_t instance, const uint8_t *data, uint32_t len)
{
    if ((data == nullptr) || (len == 0U)) {
        return BSP_ERROR_PARAM;
    }
    return sai_require(instance, BSP_SAI_FEATURE_BLOCKING_WRITE);
}

int32_t bsp_sai_read(bsp_sai_instance_t instance, uint8_t *data, uint32_t len)
{
    if ((data == nullptr) || (len == 0U)) {
        return BSP_ERROR_PARAM;
    }
    return sai_require(instance, BSP_SAI_FEATURE_BLOCKING_READ);
}

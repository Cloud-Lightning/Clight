#include "bsp_sd.h"

typedef struct {
    uintptr_t base;
    bool has_pinmux;
    uint32_t features;
} sd_map_t;

#define BSP_SD_MAP_ITEM(name, base, has_pinmux, features) { (uintptr_t)(base), (has_pinmux) != 0U, features },
static const sd_map_t s_sd_map[BSP_SD_MAX] = {
    BSP_SD_LIST(BSP_SD_MAP_ITEM)
};
#undef BSP_SD_MAP_ITEM

static int32_t sd_require(bsp_sd_instance_t instance, uint32_t feature)
{
    if (instance >= BSP_SD_MAX) {
        return BSP_ERROR_PARAM;
    }
    const sd_map_t *map = &s_sd_map[instance];
    return ((map->base != 0U) && map->has_pinmux && ((map->features & feature) != 0U)) ? BSP_OK : BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_sd_init(bsp_sd_instance_t instance)
{
    return sd_require(instance, BSP_SD_FEATURE_INIT);
}

int32_t bsp_sd_read_blocks(bsp_sd_instance_t instance, uint32_t block, uint8_t *data, uint32_t block_count)
{
    (void) block;
    if ((data == nullptr) || (block_count == 0U)) {
        return BSP_ERROR_PARAM;
    }
    return sd_require(instance, BSP_SD_FEATURE_BLOCK_READ);
}

int32_t bsp_sd_write_blocks(bsp_sd_instance_t instance, uint32_t block, const uint8_t *data, uint32_t block_count)
{
    (void) block;
    if ((data == nullptr) || (block_count == 0U)) {
        return BSP_ERROR_PARAM;
    }
    return sd_require(instance, BSP_SD_FEATURE_BLOCK_WRITE);
}

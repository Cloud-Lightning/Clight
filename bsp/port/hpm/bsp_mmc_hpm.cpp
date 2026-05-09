#include "bsp_mmc.h"

typedef struct {
    uintptr_t base;
    bool has_pinmux;
    uint32_t features;
} mmc_map_t;

#define BSP_MMC_MAP_ITEM(name, base, has_pinmux, features) { (uintptr_t)(base), (has_pinmux) != 0U, features },
static const mmc_map_t s_mmc_map[BSP_MMC_MAX] = {
    BSP_MMC_LIST(BSP_MMC_MAP_ITEM)
};
#undef BSP_MMC_MAP_ITEM

static int32_t mmc_require(bsp_mmc_instance_t instance, uint32_t feature)
{
    if (instance >= BSP_MMC_MAX) {
        return BSP_ERROR_PARAM;
    }
    const mmc_map_t *map = &s_mmc_map[instance];
    return ((map->base != 0U) && map->has_pinmux && ((map->features & feature) != 0U)) ? BSP_OK : BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_mmc_init(bsp_mmc_instance_t instance)
{
    return mmc_require(instance, BSP_MMC_FEATURE_INIT);
}

int32_t bsp_mmc_read_blocks(bsp_mmc_instance_t instance, uint32_t block, uint8_t *data, uint32_t block_count)
{
    (void) block;
    if ((data == nullptr) || (block_count == 0U)) {
        return BSP_ERROR_PARAM;
    }
    return mmc_require(instance, BSP_MMC_FEATURE_BLOCK_READ);
}

int32_t bsp_mmc_write_blocks(bsp_mmc_instance_t instance, uint32_t block, const uint8_t *data, uint32_t block_count)
{
    (void) block;
    if ((data == nullptr) || (block_count == 0U)) {
        return BSP_ERROR_PARAM;
    }
    return mmc_require(instance, BSP_MMC_FEATURE_BLOCK_WRITE);
}

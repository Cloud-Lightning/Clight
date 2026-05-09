#include "bsp_sdio.h"

typedef struct {
    uintptr_t base;
    bool has_pinmux;
    uint32_t features;
} sdio_map_t;

#define BSP_SDIO_MAP_ITEM(name, base, has_pinmux, features) { (uintptr_t)(base), (has_pinmux) != 0U, features },
static const sdio_map_t s_sdio_map[BSP_SDIO_MAX] = {
    BSP_SDIO_LIST(BSP_SDIO_MAP_ITEM)
};
#undef BSP_SDIO_MAP_ITEM

static int32_t sdio_require(bsp_sdio_instance_t instance, uint32_t feature)
{
    if (instance >= BSP_SDIO_MAX) {
        return BSP_ERROR_PARAM;
    }
    const sdio_map_t *map = &s_sdio_map[instance];
    return ((map->base != 0U) && map->has_pinmux && ((map->features & feature) != 0U)) ? BSP_OK : BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_sdio_init(bsp_sdio_instance_t instance)
{
    return sdio_require(instance, BSP_SDIO_FEATURE_INIT);
}

int32_t bsp_sdio_read_blocks(bsp_sdio_instance_t instance, uint32_t block, uint8_t *data, uint32_t block_count)
{
    (void) block;
    if ((data == nullptr) || (block_count == 0U)) {
        return BSP_ERROR_PARAM;
    }
    return sdio_require(instance, BSP_SDIO_FEATURE_BLOCK_READ);
}

int32_t bsp_sdio_write_blocks(bsp_sdio_instance_t instance, uint32_t block, const uint8_t *data, uint32_t block_count)
{
    (void) block;
    if ((data == nullptr) || (block_count == 0U)) {
        return BSP_ERROR_PARAM;
    }
    return sdio_require(instance, BSP_SDIO_FEATURE_BLOCK_WRITE);
}

#include "bsp_ospi.h"

typedef struct {
    uintptr_t base;
    bool has_pinmux;
    uint32_t features;
} ospi_map_t;

#define BSP_OSPI_MAP_ITEM(name, base, has_pinmux, features) { (uintptr_t)(base), (has_pinmux) != 0U, features },
static const ospi_map_t s_ospi_map[BSP_OSPI_MAX] = {
    BSP_OSPI_LIST(BSP_OSPI_MAP_ITEM)
};
#undef BSP_OSPI_MAP_ITEM

static int32_t ospi_require(bsp_ospi_instance_t instance, uint32_t feature)
{
    if (instance >= BSP_OSPI_MAX) {
        return BSP_ERROR_PARAM;
    }
    const ospi_map_t *map = &s_ospi_map[instance];
    return ((map->base != 0U) && map->has_pinmux && ((map->features & feature) != 0U)) ? BSP_OK : BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_ospi_init(bsp_ospi_instance_t instance)
{
    return ospi_require(instance, BSP_OSPI_FEATURE_INIT);
}

int32_t bsp_ospi_read(bsp_ospi_instance_t instance, uint32_t address, uint8_t *data, uint32_t len)
{
    (void) address;
    if ((data == nullptr) || (len == 0U)) {
        return BSP_ERROR_PARAM;
    }
    return ospi_require(instance, BSP_OSPI_FEATURE_MEM_READ);
}

int32_t bsp_ospi_write(bsp_ospi_instance_t instance, uint32_t address, const uint8_t *data, uint32_t len)
{
    (void) address;
    if ((data == nullptr) || (len == 0U)) {
        return BSP_ERROR_PARAM;
    }
    return ospi_require(instance, BSP_OSPI_FEATURE_MEM_WRITE);
}

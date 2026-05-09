#include "bsp_sdio.h"

typedef struct {
    uintptr_t base;
    bool available;
    uint32_t features;
} sdio_map_t;

#define BSP_SDIO_MAP_ITEM(name, base, available, features) { static_cast<uintptr_t>(base), (available) != 0U, static_cast<uint32_t>(features) },
static const sdio_map_t s_sdio_map[BSP_SDIO_MAX] = {
    BSP_SDIO_LIST(BSP_SDIO_MAP_ITEM)
};
#undef BSP_SDIO_MAP_ITEM

int32_t bsp_sdio_init(bsp_sdio_instance_t instance)
{
    if (instance >= BSP_SDIO_MAX) {
        return BSP_ERROR_PARAM;
    }
    return s_sdio_map[instance].available ? BSP_OK : BSP_ERROR_UNSUPPORTED;
}

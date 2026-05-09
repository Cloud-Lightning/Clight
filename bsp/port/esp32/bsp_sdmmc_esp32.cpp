#include "bsp_sdmmc.h"

typedef struct {
    uintptr_t base;
    bool available;
} sdmmc_map_t;

#define BSP_SDMMC_MAP_ITEM(name, base, available) { static_cast<uintptr_t>(base), (available) != 0U },
static const sdmmc_map_t s_sdmmc_map[BSP_SDMMC_MAX] = {
    BSP_SDMMC_LIST(BSP_SDMMC_MAP_ITEM)
};
#undef BSP_SDMMC_MAP_ITEM

int32_t bsp_sdmmc_init(bsp_sdmmc_instance_t instance)
{
    if (instance >= BSP_SDMMC_MAX) {
        return BSP_ERROR_PARAM;
    }
    return s_sdmmc_map[instance].available ? BSP_OK : BSP_ERROR_UNSUPPORTED;
}

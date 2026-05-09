#include "bsp_dfsdm.h"

typedef struct {
    uintptr_t base;
    bool available;
} dfsdm_map_t;

#define BSP_DFSDM_MAP_ITEM(name, base, available) { reinterpret_cast<uintptr_t>(base), (available) != 0U },
static const dfsdm_map_t s_dfsdm_map[BSP_DFSDM_MAX] = {
    BSP_DFSDM_LIST(BSP_DFSDM_MAP_ITEM)
};
#undef BSP_DFSDM_MAP_ITEM

int32_t bsp_dfsdm_init(bsp_dfsdm_instance_t instance)
{
    if (instance >= BSP_DFSDM_MAX) {
        return BSP_ERROR_PARAM;
    }
    return s_dfsdm_map[instance].available ? BSP_OK : BSP_ERROR_UNSUPPORTED;
}

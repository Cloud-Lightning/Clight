#include "bsp_ppa.h"

typedef struct {
    uintptr_t base;
    bool available;
} ppa_map_t;

#define BSP_PPA_MAP_ITEM(name, base, available) { static_cast<uintptr_t>(base), (available) != 0U },
static const ppa_map_t s_ppa_map[BSP_PPA_MAX] = {
    BSP_PPA_LIST(BSP_PPA_MAP_ITEM)
};
#undef BSP_PPA_MAP_ITEM

int32_t bsp_ppa_init(bsp_ppa_instance_t instance)
{
    if (instance >= BSP_PPA_MAX) {
        return BSP_ERROR_PARAM;
    }
    return s_ppa_map[instance].available ? BSP_OK : BSP_ERROR_UNSUPPORTED;
}

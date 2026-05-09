#include "bsp_parlio.h"

typedef struct {
    uintptr_t base;
    bool available;
} parlio_map_t;

#define BSP_PARLIO_MAP_ITEM(name, base, available) { static_cast<uintptr_t>(base), (available) != 0U },
static const parlio_map_t s_parlio_map[BSP_PARLIO_MAX] = {
    BSP_PARLIO_LIST(BSP_PARLIO_MAP_ITEM)
};
#undef BSP_PARLIO_MAP_ITEM

int32_t bsp_parlio_init(bsp_parlio_instance_t instance)
{
    if (instance >= BSP_PARLIO_MAX) {
        return BSP_ERROR_PARAM;
    }
    return s_parlio_map[instance].available ? BSP_OK : BSP_ERROR_UNSUPPORTED;
}

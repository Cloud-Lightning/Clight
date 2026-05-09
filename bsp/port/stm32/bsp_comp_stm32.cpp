#include "bsp_comp.h"

typedef struct {
    uintptr_t base;
    bool available;
} comp_map_t;

#define BSP_COMP_MAP_ITEM(name, base, available) { reinterpret_cast<uintptr_t>(base), (available) != 0U },
static const comp_map_t s_comp_map[BSP_COMP_MAX] = {
    BSP_COMP_LIST(BSP_COMP_MAP_ITEM)
};
#undef BSP_COMP_MAP_ITEM

int32_t bsp_comp_init(bsp_comp_instance_t instance)
{
    if (instance >= BSP_COMP_MAX) {
        return BSP_ERROR_PARAM;
    }
    return s_comp_map[instance].available ? BSP_OK : BSP_ERROR_UNSUPPORTED;
}

#include "bsp_opamp.h"

typedef struct {
    uintptr_t base;
    bool available;
} opamp_map_t;

#define BSP_OPAMP_MAP_ITEM(name, base, available) { reinterpret_cast<uintptr_t>(base), (available) != 0U },
static const opamp_map_t s_opamp_map[BSP_OPAMP_MAX] = {
    BSP_OPAMP_LIST(BSP_OPAMP_MAP_ITEM)
};
#undef BSP_OPAMP_MAP_ITEM

int32_t bsp_opamp_init(bsp_opamp_instance_t instance)
{
    if (instance >= BSP_OPAMP_MAX) {
        return BSP_ERROR_PARAM;
    }
    return s_opamp_map[instance].available ? BSP_OK : BSP_ERROR_UNSUPPORTED;
}

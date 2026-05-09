#include "bsp_hcd.h"

typedef struct {
    uintptr_t base;
    bool available;
} hcd_map_t;

#define BSP_HCD_MAP_ITEM(name, base, available) { reinterpret_cast<uintptr_t>(base), (available) != 0U },
static const hcd_map_t s_hcd_map[BSP_HCD_MAX] = {
    BSP_HCD_LIST(BSP_HCD_MAP_ITEM)
};
#undef BSP_HCD_MAP_ITEM

int32_t bsp_hcd_init(bsp_hcd_instance_t instance)
{
    if (instance >= BSP_HCD_MAX) {
        return BSP_ERROR_PARAM;
    }
    return s_hcd_map[instance].available ? BSP_OK : BSP_ERROR_UNSUPPORTED;
}

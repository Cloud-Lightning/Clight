#include "bsp_dcmi.h"

typedef struct {
    uintptr_t base;
    bool available;
} dcmi_map_t;

#define BSP_DCMI_MAP_ITEM(name, base, available) { reinterpret_cast<uintptr_t>(base), (available) != 0U },
static const dcmi_map_t s_dcmi_map[BSP_DCMI_MAX] = {
    BSP_DCMI_LIST(BSP_DCMI_MAP_ITEM)
};
#undef BSP_DCMI_MAP_ITEM

int32_t bsp_dcmi_init(bsp_dcmi_instance_t instance)
{
    if (instance >= BSP_DCMI_MAX) {
        return BSP_ERROR_PARAM;
    }
    return s_dcmi_map[instance].available ? BSP_OK : BSP_ERROR_UNSUPPORTED;
}

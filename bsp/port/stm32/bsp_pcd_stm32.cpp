#include "bsp_pcd.h"

typedef struct {
    uintptr_t base;
    bool available;
} pcd_map_t;

#define BSP_PCD_MAP_ITEM(name, base, available) { reinterpret_cast<uintptr_t>(base), (available) != 0U },
static const pcd_map_t s_pcd_map[BSP_PCD_MAX] = {
    BSP_PCD_LIST(BSP_PCD_MAP_ITEM)
};
#undef BSP_PCD_MAP_ITEM

int32_t bsp_pcd_init(bsp_pcd_instance_t instance)
{
    if (instance >= BSP_PCD_MAX) {
        return BSP_ERROR_PARAM;
    }
    return s_pcd_map[instance].available ? BSP_OK : BSP_ERROR_UNSUPPORTED;
}

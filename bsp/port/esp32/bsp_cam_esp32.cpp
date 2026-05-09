#include "bsp_cam.h"

typedef struct {
    uintptr_t base;
    bool available;
} cam_map_t;

#define BSP_CAM_MAP_ITEM(name, base, available) { static_cast<uintptr_t>(base), (available) != 0U },
static const cam_map_t s_cam_map[BSP_CAM_MAX] = {
    BSP_CAM_LIST(BSP_CAM_MAP_ITEM)
};
#undef BSP_CAM_MAP_ITEM

int32_t bsp_cam_init(bsp_cam_instance_t instance)
{
    if (instance >= BSP_CAM_MAX) {
        return BSP_ERROR_PARAM;
    }
    return s_cam_map[instance].available ? BSP_OK : BSP_ERROR_UNSUPPORTED;
}

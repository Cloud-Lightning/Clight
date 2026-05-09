#include "bsp_jpeg.h"

typedef struct {
    uintptr_t base;
    bool available;
} jpeg_map_t;

#define BSP_JPEG_MAP_ITEM(name, base, available) { static_cast<uintptr_t>(base), (available) != 0U },
static const jpeg_map_t s_jpeg_map[BSP_JPEG_MAX] = {
    BSP_JPEG_LIST(BSP_JPEG_MAP_ITEM)
};
#undef BSP_JPEG_MAP_ITEM

int32_t bsp_jpeg_init(bsp_jpeg_instance_t instance)
{
    if (instance >= BSP_JPEG_MAX) {
        return BSP_ERROR_PARAM;
    }
    return s_jpeg_map[instance].available ? BSP_OK : BSP_ERROR_UNSUPPORTED;
}

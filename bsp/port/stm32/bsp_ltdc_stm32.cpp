#include "bsp_ltdc.h"

typedef struct {
    uintptr_t base;
    bool available;
} ltdc_map_t;

#define BSP_LTDC_MAP_ITEM(name, base, available) { reinterpret_cast<uintptr_t>(base), (available) != 0U },
static const ltdc_map_t s_ltdc_map[BSP_LTDC_MAX] = {
    BSP_LTDC_LIST(BSP_LTDC_MAP_ITEM)
};
#undef BSP_LTDC_MAP_ITEM

int32_t bsp_ltdc_init(bsp_ltdc_instance_t instance)
{
    if (instance >= BSP_LTDC_MAX) {
        return BSP_ERROR_PARAM;
    }
    return s_ltdc_map[instance].available ? BSP_OK : BSP_ERROR_UNSUPPORTED;
}

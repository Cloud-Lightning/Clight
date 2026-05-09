#include "bsp_pssi.h"

typedef struct {
    uintptr_t base;
    bool available;
} pssi_map_t;

#define BSP_PSSI_MAP_ITEM(name, base, available) { reinterpret_cast<uintptr_t>(base), (available) != 0U },
static const pssi_map_t s_pssi_map[BSP_PSSI_MAX] = {
    BSP_PSSI_LIST(BSP_PSSI_MAP_ITEM)
};
#undef BSP_PSSI_MAP_ITEM

int32_t bsp_pssi_init(bsp_pssi_instance_t instance)
{
    if (instance >= BSP_PSSI_MAX) {
        return BSP_ERROR_PARAM;
    }
    return s_pssi_map[instance].available ? BSP_OK : BSP_ERROR_UNSUPPORTED;
}

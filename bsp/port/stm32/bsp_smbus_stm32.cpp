#include "bsp_smbus.h"

typedef struct {
    uintptr_t base;
    bool available;
} smbus_map_t;

#define BSP_SMBUS_MAP_ITEM(name, base, available) { reinterpret_cast<uintptr_t>(base), (available) != 0U },
static const smbus_map_t s_smbus_map[BSP_SMBUS_MAX] = {
    BSP_SMBUS_LIST(BSP_SMBUS_MAP_ITEM)
};
#undef BSP_SMBUS_MAP_ITEM

int32_t bsp_smbus_init(bsp_smbus_instance_t instance)
{
    if (instance >= BSP_SMBUS_MAX) {
        return BSP_ERROR_PARAM;
    }
    return s_smbus_map[instance].available ? BSP_OK : BSP_ERROR_UNSUPPORTED;
}

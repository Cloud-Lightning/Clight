#include "bsp_smartcard.h"

typedef struct {
    uintptr_t base;
    bool available;
} smartcard_map_t;

#define BSP_SMARTCARD_MAP_ITEM(name, base, available) { reinterpret_cast<uintptr_t>(base), (available) != 0U },
static const smartcard_map_t s_smartcard_map[BSP_SMARTCARD_MAX] = {
    BSP_SMARTCARD_LIST(BSP_SMARTCARD_MAP_ITEM)
};
#undef BSP_SMARTCARD_MAP_ITEM

int32_t bsp_smartcard_init(bsp_smartcard_instance_t instance)
{
    if (instance >= BSP_SMARTCARD_MAX) {
        return BSP_ERROR_PARAM;
    }
    return s_smartcard_map[instance].available ? BSP_OK : BSP_ERROR_UNSUPPORTED;
}

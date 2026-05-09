#include "bsp_usart.h"

typedef struct {
    uintptr_t base;
    bool available;
} usart_map_t;

#define BSP_USART_MAP_ITEM(name, base, available) { reinterpret_cast<uintptr_t>(base), (available) != 0U },
static const usart_map_t s_usart_map[BSP_USART_MAX] = {
    BSP_USART_LIST(BSP_USART_MAP_ITEM)
};
#undef BSP_USART_MAP_ITEM

int32_t bsp_usart_init(bsp_usart_instance_t instance)
{
    if (instance >= BSP_USART_MAX) {
        return BSP_ERROR_PARAM;
    }
    return s_usart_map[instance].available ? BSP_OK : BSP_ERROR_UNSUPPORTED;
}

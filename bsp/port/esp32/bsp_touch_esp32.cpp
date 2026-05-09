#include "bsp_touch.h"

int32_t bsp_touch_init(bsp_touch_instance_t instance)
{
    if (instance >= BSP_TOUCH_MAX) {
        return BSP_ERROR_PARAM;
    }

    return BSP_ERROR_UNSUPPORTED;
}

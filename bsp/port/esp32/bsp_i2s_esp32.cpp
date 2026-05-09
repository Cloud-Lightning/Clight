#include "bsp_i2s.h"

int32_t bsp_i2s_init(bsp_i2s_instance_t instance)
{
    if (instance >= BSP_I2S_MAX) {
        return BSP_ERROR_PARAM;
    }

    return BSP_ERROR_UNSUPPORTED;
}

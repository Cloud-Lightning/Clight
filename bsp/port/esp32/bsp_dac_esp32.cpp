#include "bsp_dac.h"

int32_t bsp_dac_init(bsp_dac_instance_t instance)
{
    if (instance >= BSP_DAC_MAX) {
        return BSP_ERROR_PARAM;
    }

    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_dac_write(bsp_dac_instance_t instance, uint8_t channel, uint32_t value)
{
    (void) channel;
    (void) value;
    if (instance >= BSP_DAC_MAX) {
        return BSP_ERROR_PARAM;
    }

    return BSP_ERROR_UNSUPPORTED;
}

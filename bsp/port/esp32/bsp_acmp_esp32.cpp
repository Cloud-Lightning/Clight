#include "bsp_acmp.h"

int32_t bsp_acmp_init(bsp_acmp_instance_t instance, uint8_t channel)
{
    (void) channel;
    if (instance >= BSP_ACMP_MAX) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_acmp_get_status(bsp_acmp_instance_t instance, uint8_t channel, uint32_t *status_out)
{
    (void) channel;
    if ((instance >= BSP_ACMP_MAX) || (status_out == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR_UNSUPPORTED;
}

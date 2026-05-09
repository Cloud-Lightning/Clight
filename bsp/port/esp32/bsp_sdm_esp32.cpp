#include "bsp_sdm.h"

int32_t bsp_sdm_init(bsp_sdm_instance_t instance, uint8_t channel)
{
    (void) channel;
    if (instance >= BSP_SDM_MAX) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_sdm_get_status(bsp_sdm_instance_t instance, uint32_t *status_out)
{
    if ((instance >= BSP_SDM_MAX) || (status_out == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR_UNSUPPORTED;
}

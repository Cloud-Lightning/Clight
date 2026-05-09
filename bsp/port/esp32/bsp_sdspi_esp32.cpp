#include "bsp_sdspi.h"

int32_t bsp_sdspi_init(bsp_sdspi_instance_t instance)
{
    if (instance >= BSP_SDSPI_MAX) {
        return BSP_ERROR_PARAM;
    }

    return BSP_ERROR_UNSUPPORTED;
}

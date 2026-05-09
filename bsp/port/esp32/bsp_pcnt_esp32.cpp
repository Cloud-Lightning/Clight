#include "bsp_pcnt.h"

int32_t bsp_pcnt_init(bsp_pcnt_instance_t instance)
{
    if (instance >= BSP_PCNT_MAX) {
        return BSP_ERROR_PARAM;
    }

    return BSP_ERROR_UNSUPPORTED;
}

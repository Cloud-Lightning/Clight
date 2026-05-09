#include "bsp_rmt.h"

int32_t bsp_rmt_init(bsp_rmt_instance_t instance)
{
    if (instance >= BSP_RMT_MAX) {
        return BSP_ERROR_PARAM;
    }

    return BSP_ERROR_UNSUPPORTED;
}

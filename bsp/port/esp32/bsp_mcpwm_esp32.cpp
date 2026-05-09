#include "bsp_mcpwm.h"

int32_t bsp_mcpwm_init(bsp_mcpwm_instance_t instance)
{
    if (instance >= BSP_MCPWM_MAX) {
        return BSP_ERROR_PARAM;
    }

    return BSP_ERROR_UNSUPPORTED;
}

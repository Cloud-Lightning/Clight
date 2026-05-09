/**
 * @file bsp_l1c_hpm.cpp
 * @brief HPM platform implementation for BSP L1C interface
 */

#include "bsp_l1c.h"

extern "C" {
#include "hpm_l1c_drv.h"
}

int32_t bsp_l1c_get_status(bsp_l1c_instance_t instance, bool *ic_enabled, bool *dc_enabled)
{
    if ((instance >= BSP_L1C_MAX) || (ic_enabled == nullptr) || (dc_enabled == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    *ic_enabled = l1c_ic_is_enabled();
    *dc_enabled = l1c_dc_is_enabled();
    return BSP_OK;
}

int32_t bsp_l1c_invalidate_dcache_all(bsp_l1c_instance_t instance)
{
    if (instance >= BSP_L1C_MAX) {
        return BSP_ERROR_PARAM;
    }

    l1c_dc_invalidate_all();
    return BSP_OK;
}

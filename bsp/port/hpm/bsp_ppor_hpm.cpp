/**
 * @file bsp_ppor_hpm.cpp
 * @brief HPM platform implementation for BSP PPOR interface
 */

#include "bsp_ppor.h"

extern "C" {
#include "hpm_ppor_drv.h"
}

typedef struct {
    PPOR_Type *base;
} ppor_instance_map_t;

#define BSP_PPOR_MAP_ITEM(name, base_addr) { base_addr },
static const ppor_instance_map_t s_ppor_map[BSP_PPOR_MAX] = {
    BSP_PPOR_LIST(BSP_PPOR_MAP_ITEM)
};
#undef BSP_PPOR_MAP_ITEM

int32_t bsp_ppor_get_reset_flags(bsp_ppor_instance_t instance, uint32_t *flags_out)
{
    if ((instance >= BSP_PPOR_MAX) || (flags_out == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    *flags_out = ppor_reset_get_flags(s_ppor_map[instance].base);
    return BSP_OK;
}

int32_t bsp_ppor_clear_reset_flags(bsp_ppor_instance_t instance, uint32_t mask)
{
    if (instance >= BSP_PPOR_MAX) {
        return BSP_ERROR_PARAM;
    }

    ppor_reset_clear_flags(s_ppor_map[instance].base, mask);
    return BSP_OK;
}

/**
 * @file bsp_pllctlv2_hpm.cpp
 * @brief HPM platform implementation for BSP PLLCTLV2 interface
 */

#include "bsp_pllctlv2.h"

extern "C" {
#include "hpm_pllctlv2_drv.h"
}

typedef struct {
    PLLCTLV2_Type *base;
} pllctlv2_instance_map_t;

#define BSP_PLLCTLV2_MAP_ITEM(name, base_addr) { base_addr },
static const pllctlv2_instance_map_t s_pllctlv2_map[BSP_PLLCTLV2_MAX] = {
    BSP_PLLCTLV2_LIST(BSP_PLLCTLV2_MAP_ITEM)
};
#undef BSP_PLLCTLV2_MAP_ITEM

int32_t bsp_pllctlv2_get_pll_freq(bsp_pllctlv2_instance_t instance, uint8_t pll_index, uint32_t *freq_out)
{
    if ((instance >= BSP_PLLCTLV2_MAX) || (freq_out == nullptr) || (pll_index > (uint8_t) pllctlv2_pll6)) {
        return BSP_ERROR_PARAM;
    }

    *freq_out = pllctlv2_get_pll_freq_in_hz(s_pllctlv2_map[instance].base, (pllctlv2_pll_t) pll_index);
    return BSP_OK;
}

int32_t bsp_pllctlv2_is_pll_stable(bsp_pllctlv2_instance_t instance, uint8_t pll_index, bool *stable_out)
{
    if ((instance >= BSP_PLLCTLV2_MAX) || (stable_out == nullptr) || (pll_index > (uint8_t) pllctlv2_pll6)) {
        return BSP_ERROR_PARAM;
    }

    *stable_out = pllctlv2_pll_is_stable(s_pllctlv2_map[instance].base, (pllctlv2_pll_t) pll_index);
    return BSP_OK;
}

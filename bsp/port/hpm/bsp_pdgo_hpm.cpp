/**
 * @file bsp_pdgo_hpm.cpp
 * @brief HPM platform implementation for BSP PDGO interface
 */

#include "bsp_pdgo.h"

extern "C" {
#include "hpm_pdgo_drv.h"
}

typedef struct {
    PDGO_Type *base;
} pdgo_instance_map_t;

#define BSP_PDGO_MAP_ITEM(name, base_addr) { base_addr },
static const pdgo_instance_map_t s_pdgo_map[BSP_PDGO_MAX] = {
    BSP_PDGO_LIST(BSP_PDGO_MAP_ITEM)
};
#undef BSP_PDGO_MAP_ITEM

int32_t bsp_pdgo_init(bsp_pdgo_instance_t instance)
{
    if (instance >= BSP_PDGO_MAX) {
        return BSP_ERROR_PARAM;
    }

    PDGO_Type *base = s_pdgo_map[instance].base;

    pdgo_disable_all_irq(base);
    pdgo_clear_wakeup_status(base, 0xFFFFFFFFU);
    pdgo_disable_software_wakeup(base);
    return BSP_OK;
}

int32_t bsp_pdgo_get_wakeup_status(bsp_pdgo_instance_t instance, uint32_t *status_out)
{
    if ((instance >= BSP_PDGO_MAX) || (status_out == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    *status_out = pdgo_get_wakeup_status(s_pdgo_map[instance].base);
    return BSP_OK;
}

int32_t bsp_pdgo_write_gpr(bsp_pdgo_instance_t instance, uint32_t index, uint32_t value)
{
    if ((instance >= BSP_PDGO_MAX) || (index >= BSP_PDGO_MAIN_GPR_COUNT)) {
        return BSP_ERROR_PARAM;
    }

    pdgo_write_gpr(s_pdgo_map[instance].base, index, value);
    return BSP_OK;
}

int32_t bsp_pdgo_read_gpr(bsp_pdgo_instance_t instance, uint32_t index, uint32_t *value_out)
{
    if ((instance >= BSP_PDGO_MAX) || (index >= BSP_PDGO_MAIN_GPR_COUNT) || (value_out == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    *value_out = pdgo_read_gpr(s_pdgo_map[instance].base, index);
    return BSP_OK;
}

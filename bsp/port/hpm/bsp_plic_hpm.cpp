/**
 * @file bsp_plic_hpm.cpp
 * @brief HPM platform implementation for BSP PLIC interface
 */

#include "bsp_plic.h"

extern "C" {
#include "hpm_plic_drv.h"
}

typedef struct {
    PLIC_Type *base;
} plic_instance_map_t;

#define BSP_PLIC_MAP_ITEM(name, base_addr) { base_addr },
static const plic_instance_map_t s_plic_map[BSP_PLIC_MAX] = {
    BSP_PLIC_LIST(BSP_PLIC_MAP_ITEM)
};
#undef BSP_PLIC_MAP_ITEM

int32_t bsp_plic_init(bsp_plic_instance_t instance)
{
    if (instance >= BSP_PLIC_MAX) {
        return BSP_ERROR_PARAM;
    }

    __plic_set_threshold((uint32_t) s_plic_map[instance].base, HPM_PLIC_TARGET_M_MODE, 0U);
    return BSP_OK;
}

int32_t bsp_plic_config_irq(bsp_plic_instance_t instance, uint32_t irq, uint32_t priority, bsp_state_t state)
{
    if ((instance >= BSP_PLIC_MAX) || (irq == 0U)) {
        return BSP_ERROR_PARAM;
    }

    uint32_t base = (uint32_t) s_plic_map[instance].base;

    __plic_set_irq_priority(base, irq, priority);
    if (state == BSP_ENABLE) {
        __plic_enable_irq(base, HPM_PLIC_TARGET_M_MODE, irq);
    } else {
        __plic_disable_irq(base, HPM_PLIC_TARGET_M_MODE, irq);
    }
    return BSP_OK;
}

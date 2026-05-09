/**
 * @file bsp_plb_hpm.cpp
 * @brief HPM platform implementation for BSP PLB interface
 */

#include "bsp_plb.h"
#include "board.h"

extern "C" {
#include "hpm_clock_drv.h"
#include "hpm_plb_drv.h"
}

typedef struct {
    PLB_Type *base;
    clock_name_t clock_name;
} plb_instance_map_t;

#define BSP_PLB_MAP_ITEM(name, base_addr, clk_name) { base_addr, clk_name },
static const plb_instance_map_t s_plb_map[BSP_PLB_MAX] = {
    BSP_PLB_LIST(BSP_PLB_MAP_ITEM)
};
#undef BSP_PLB_MAP_ITEM

int32_t bsp_plb_init(bsp_plb_instance_t instance)
{
    if (instance >= BSP_PLB_MAX) {
        return BSP_ERROR_PARAM;
    }

    const plb_instance_map_t *map = &s_plb_map[instance];

    clock_add_to_group(map->clock_name, BOARD_RUNNING_CORE & 0x1);
    plb_type_b_set_all_slice(map->base, plb_chn0, plb_slice_opt_keep);
    return BSP_OK;
}

int32_t bsp_plb_get_counter(bsp_plb_instance_t instance, uint8_t channel, uint32_t *counter_out)
{
    if ((instance >= BSP_PLB_MAX) || (counter_out == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    *counter_out = plb_type_b_get_counter(s_plb_map[instance].base, (plb_chn_t) channel);
    return BSP_OK;
}

/**
 * @file bsp_lobs_hpm.cpp
 * @brief HPM platform implementation for BSP LOBS interface
 */

#include "bsp_lobs.h"
#include "board.h"
#include <stdint.h>

extern "C" {
#include "hpm_clock_drv.h"
#include "hpm_lobs_drv.h"
}

typedef struct {
    LOBS_Type *base;
    clock_name_t clock_name;
} lobs_instance_map_t;

#define BSP_LOBS_MAP_ITEM(name, base_addr, clk_name) { base_addr, clk_name },
static const lobs_instance_map_t s_lobs_map[BSP_LOBS_MAX] = {
    BSP_LOBS_LIST(BSP_LOBS_MAP_ITEM)
};
#undef BSP_LOBS_MAP_ITEM

ATTR_PLACE_AT(".ahb_sram") static lobs_trace_data_t s_lobs_buffer[128];

int32_t bsp_lobs_init(bsp_lobs_instance_t instance)
{
    if (instance >= BSP_LOBS_MAX) {
        return BSP_ERROR_PARAM;
    }

    const lobs_instance_map_t *map = &s_lobs_map[instance];
    lobs_ctrl_config_t ctrl_config;

    clock_add_to_group(map->clock_name, BOARD_RUNNING_CORE & 0x1);
    lobs_unlock(map->base);
    lobs_deinit(map->base);

    ctrl_config.group_mode = lobs_one_group_96_bits;
    ctrl_config.sample_rate = lobs_sample_1_per_10;
    ctrl_config.start_addr = (uint32_t) (uintptr_t) &s_lobs_buffer[0];
    ctrl_config.end_addr = ctrl_config.start_addr + sizeof(s_lobs_buffer);
    lobs_ctrl_config(map->base, &ctrl_config);
    lobs_lock(map->base);
    return BSP_OK;
}

int32_t bsp_lobs_enable(bsp_lobs_instance_t instance, bsp_state_t state)
{
    if (instance >= BSP_LOBS_MAX) {
        return BSP_ERROR_PARAM;
    }

    lobs_set_enable(s_lobs_map[instance].base, state == BSP_ENABLE);
    return BSP_OK;
}

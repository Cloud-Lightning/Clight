/**
 * @file bsp_ptpc_hpm.cpp
 * @brief HPM platform implementation for BSP PTPC interface
 */

#include "bsp_ptpc.h"
#include "board.h"
#include <string.h>

extern "C" {
#include "hpm_clock_drv.h"
#include "hpm_ptpc_drv.h"
}

typedef struct {
    PTPC_Type *base;
    uint32_t irq;
    clock_name_t clock_name;
} ptpc_instance_map_t;

#define BSP_PTPC_MAP_ITEM(name, base_addr, irq_num, clk_name) { base_addr, irq_num, clk_name },
static const ptpc_instance_map_t s_ptpc_map[BSP_PTPC_MAX] = {
    BSP_PTPC_LIST(BSP_PTPC_MAP_ITEM)
};
#undef BSP_PTPC_MAP_ITEM

int32_t bsp_ptpc_init(bsp_ptpc_instance_t instance, uint8_t timer_index)
{
    if (instance >= BSP_PTPC_MAX) {
        return BSP_ERROR_PARAM;
    }

    const ptpc_instance_map_t *map = &s_ptpc_map[instance];
    ptpc_config_t config;

    clock_add_to_group(map->clock_name, BOARD_RUNNING_CORE & 0x1);
    memset(&config, 0, sizeof(config));
    ptpc_get_default_config(map->base, &config);
    config.coarse_increment = false;
    config.capture_keep = false;
    config.ns_rollover_mode = ptpc_ns_counter_rollover_digital;
    config.src_frequency = clock_get_frequency(map->clock_name);
    if (ptpc_init(map->base, timer_index, &config) != status_success) {
        return BSP_ERROR;
    }

    ptpc_init_timer(map->base, timer_index);
    return BSP_OK;
}

int32_t bsp_ptpc_get_time(bsp_ptpc_instance_t instance, uint8_t timer_index, uint32_t *sec_out, uint32_t *nsec_out)
{
    if ((instance >= BSP_PTPC_MAX) || (sec_out == nullptr) || (nsec_out == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    const ptpc_instance_map_t *map = &s_ptpc_map[instance];

    *sec_out = ptpc_get_timestamp_second(map->base, timer_index);
    *nsec_out = ptpc_get_timestamp_ns(map->base, timer_index);
    return BSP_OK;
}

/**
 * @file bsp_owr_hpm.cpp
 * @brief HPM platform implementation for BSP OWR interface
 */

#include "bsp_owr.h"
#include "board.h"
#include <string.h>

extern "C" {
#include "hpm_clock_drv.h"
#include "hpm_owr_drv.h"
}

typedef struct {
    OWR_Type *base;
    clock_name_t clock_name;
    uint8_t has_pinmux;
} owr_instance_map_t;

#define BSP_OWR_MAP_ITEM(name, base_addr, clk_name, has_pinmux_flag) { base_addr, clk_name, has_pinmux_flag },
static const owr_instance_map_t s_owr_map[BSP_OWR_MAX] = {
    BSP_OWR_LIST(BSP_OWR_MAP_ITEM)
};
#undef BSP_OWR_MAP_ITEM

int32_t bsp_owr_init(bsp_owr_instance_t instance)
{
    if (instance >= BSP_OWR_MAX) {
        return BSP_ERROR_PARAM;
    }

    const owr_instance_map_t *map = &s_owr_map[instance];
    if (map->has_pinmux == 0U) {
        return BSP_ERROR_UNSUPPORTED;
    }

    owr_config_t config;

    memset(&config, 0, sizeof(config));
    clock_add_to_group(map->clock_name, BOARD_RUNNING_CORE & 0x1);
    config.clock_source_frequency = clock_get_frequency(map->clock_name);
    if (owr_init(map->base, &config) != status_success) {
        return BSP_ERROR;
    }

    return BSP_OK;
}

int32_t bsp_owr_get_presence(bsp_owr_instance_t instance, uint32_t *status_out)
{
    if ((instance >= BSP_OWR_MAX) || (status_out == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    const owr_instance_map_t *map = &s_owr_map[instance];
    if (map->has_pinmux == 0U) {
        return BSP_ERROR_UNSUPPORTED;
    }

    if (owr_get_presence_status(map->base, status_out) != status_success) {
        return BSP_ERROR;
    }

    return BSP_OK;
}

/**
 * @file bsp_acmp_hpm.cpp
 * @brief HPM platform implementation for BSP ACMP interface
 */

#include "bsp_acmp.h"
#include "board.h"

extern "C" {
#include "hpm_acmp_drv.h"
#include "hpm_clock_drv.h"
}

typedef struct {
    ACMP_Type *base;
    clock_name_t clock_name;
    uint8_t has_pinmux;
} acmp_instance_map_t;

#define BSP_ACMP_MAP_ITEM(name, base_addr, clk_name, has_pinmux_flag) { base_addr, clk_name, has_pinmux_flag },
static const acmp_instance_map_t s_acmp_map[BSP_ACMP_MAX] = {
    BSP_ACMP_LIST(BSP_ACMP_MAP_ITEM)
};
#undef BSP_ACMP_MAP_ITEM

int32_t bsp_acmp_init(bsp_acmp_instance_t instance, uint8_t channel)
{
    if (instance >= BSP_ACMP_MAX) {
        return BSP_ERROR_PARAM;
    }

    const acmp_instance_map_t *map = &s_acmp_map[instance];
    if (map->has_pinmux == 0U) {
        return BSP_ERROR_UNSUPPORTED;
    }

    acmp_channel_config_t config;

    clock_add_to_group(map->clock_name, BOARD_RUNNING_CORE & 0x1);
    acmp_channel_get_default_config(map->base, &config);
    if (acmp_channel_config(map->base, channel, &config, true) != status_success) {
        return BSP_ERROR;
    }

    return BSP_OK;
}

int32_t bsp_acmp_get_status(bsp_acmp_instance_t instance, uint8_t channel, uint32_t *status_out)
{
    if ((instance >= BSP_ACMP_MAX) || (status_out == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    *status_out = acmp_channel_get_status(s_acmp_map[instance].base, channel);
    return BSP_OK;
}

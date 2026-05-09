/**
 * @file bsp_sdm_hpm.cpp
 * @brief HPM platform implementation for BSP SDM interface
 */

#include "bsp_sdm.h"
#include "board.h"

extern "C" {
#include "hpm_clock_drv.h"
#include "hpm_sdm_drv.h"
}

typedef struct {
    SDM_Type *base;
    clock_name_t clock_name;
    uint8_t has_pinmux;
} sdm_instance_map_t;

#define BSP_SDM_MAP_ITEM(name, base_addr, clk_name, has_pinmux_flag) { base_addr, clk_name, has_pinmux_flag },
static const sdm_instance_map_t s_sdm_map[BSP_SDM_MAX] = {
    BSP_SDM_LIST(BSP_SDM_MAP_ITEM)
};
#undef BSP_SDM_MAP_ITEM

int32_t bsp_sdm_init(bsp_sdm_instance_t instance, uint8_t channel)
{
    if (instance >= BSP_SDM_MAX) {
        return BSP_ERROR_PARAM;
    }

    const sdm_instance_map_t *map = &s_sdm_map[instance];
    if (map->has_pinmux == 0U) {
        return BSP_ERROR_UNSUPPORTED;
    }

    sdm_control_t control;
    sdm_channel_common_config_t common_config;

    clock_add_to_group(map->clock_name, BOARD_RUNNING_CORE & 0x1);
    sdm_get_default_module_control(map->base, &control);
    sdm_init_module(map->base, &control);
    sdm_get_channel_common_setting(map->base, &common_config);
    sdm_config_channel_common_setting(map->base, channel, &common_config);
    return BSP_OK;
}

int32_t bsp_sdm_get_status(bsp_sdm_instance_t instance, uint32_t *status_out)
{
    if ((instance >= BSP_SDM_MAX) || (status_out == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    *status_out = sdm_get_status(s_sdm_map[instance].base);
    return BSP_OK;
}

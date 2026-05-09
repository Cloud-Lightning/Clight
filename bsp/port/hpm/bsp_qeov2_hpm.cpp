/**
 * @file bsp_qeov2_hpm.cpp
 * @brief HPM platform implementation for BSP QEOV2 interface
 */

#include "bsp_qeov2.h"
#include "board.h"
#include <string.h>

extern "C" {
#include "hpm_clock_drv.h"
#include "hpm_qeov2_drv.h"
}

typedef struct {
    QEOV2_Type *base;
    clock_name_t clock_name;
    uint8_t has_pinmux;
} qeov2_instance_map_t;

#define BSP_QEOV2_MAP_ITEM(name, base_addr, clk_name, has_pinmux_flag) { base_addr, clk_name, has_pinmux_flag },
static const qeov2_instance_map_t s_qeov2_map[BSP_QEOV2_MAX] = {
    BSP_QEOV2_LIST(BSP_QEOV2_MAP_ITEM)
};
#undef BSP_QEOV2_MAP_ITEM

int32_t bsp_qeov2_init(bsp_qeov2_instance_t instance)
{
    if (instance >= BSP_QEOV2_MAX) {
        return BSP_ERROR_PARAM;
    }

    const qeov2_instance_map_t *map = &s_qeov2_map[instance];
    if (map->has_pinmux == 0U) {
        return BSP_ERROR_UNSUPPORTED;
    }

    qeo_abz_mode_t mode;

    clock_add_to_group(map->clock_name, BOARD_RUNNING_CORE & 0x1);
    memset(&mode, 0, sizeof(mode));
    qeo_abz_get_default_mode_config(map->base, &mode);
    qeo_abz_config_mode(map->base, &mode);
    qeo_enable_software_position_inject(map->base);
    return BSP_OK;
}

int32_t bsp_qeov2_start(bsp_qeov2_instance_t instance)
{
    if (instance >= BSP_QEOV2_MAX) {
        return BSP_ERROR_PARAM;
    }

    if (s_qeov2_map[instance].has_pinmux == 0U) {
        return BSP_ERROR_UNSUPPORTED;
    }

    qeo_abz_enable_output(s_qeov2_map[instance].base);
    return BSP_OK;
}

int32_t bsp_qeov2_stop(bsp_qeov2_instance_t instance)
{
    if (instance >= BSP_QEOV2_MAX) {
        return BSP_ERROR_PARAM;
    }

    if (s_qeov2_map[instance].has_pinmux == 0U) {
        return BSP_ERROR_UNSUPPORTED;
    }

    qeo_abz_disable_output(s_qeov2_map[instance].base);
    return BSP_OK;
}

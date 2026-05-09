/**
 * @file bsp_ppi_hpm.cpp
 * @brief HPM platform implementation for BSP PPI interface
 */

#include "bsp_ppi.h"
#include "board.h"

extern "C" {
#include "hpm_clock_drv.h"
#include "hpm_ppi_drv.h"
}

typedef struct {
    PPI_Type *base;
    clock_name_t clock_name;
    uint8_t has_pinmux;
} ppi_instance_map_t;

#define BSP_PPI_MAP_ITEM(name, base_addr, clk_name, has_pinmux_flag) { base_addr, clk_name, has_pinmux_flag },
static const ppi_instance_map_t s_ppi_map[BSP_PPI_MAX] = {
    BSP_PPI_LIST(BSP_PPI_MAP_ITEM)
};
#undef BSP_PPI_MAP_ITEM

int32_t bsp_ppi_init(bsp_ppi_instance_t instance)
{
    if (instance >= BSP_PPI_MAX) {
        return BSP_ERROR_PARAM;
    }

    const ppi_instance_map_t *map = &s_ppi_map[instance];
    if (map->has_pinmux == 0U) {
        return BSP_ERROR_UNSUPPORTED;
    }

    clock_add_to_group(map->clock_name, BOARD_RUNNING_CORE & 0x1);
    ppi_set_reset(map->base, true);
    ppi_set_reset(map->base, false);
    return BSP_OK;
}

int32_t bsp_ppi_config_timeout(bsp_ppi_instance_t instance, uint16_t timeout_count, bsp_state_t state)
{
    if (instance >= BSP_PPI_MAX) {
        return BSP_ERROR_PARAM;
    }

    const ppi_instance_map_t *map = &s_ppi_map[instance];
    if (map->has_pinmux == 0U) {
        return BSP_ERROR_UNSUPPORTED;
    }

    ppi_config_timeout(map->base, timeout_count, state == BSP_ENABLE);
    return BSP_OK;
}

/**
 * @file bsp_eui_hpm.cpp
 * @brief HPM platform implementation for BSP EUI interface
 */

#include "bsp_eui.h"
#include "board.h"

extern "C" {
#include "hpm_clock_drv.h"
#include "hpm_eui_drv.h"
}

typedef struct {
    EUI_Type *base;
    clock_name_t clock_name;
    uint8_t has_pinmux;
} eui_instance_map_t;

#define BSP_EUI_MAP_ITEM(name, base_addr, clk_name, has_pinmux_flag) { base_addr, clk_name, has_pinmux_flag },
static const eui_instance_map_t s_eui_map[BSP_EUI_MAX] = {
    BSP_EUI_LIST(BSP_EUI_MAP_ITEM)
};
#undef BSP_EUI_MAP_ITEM

int32_t bsp_eui_init(bsp_eui_instance_t instance)
{
    if (instance >= BSP_EUI_MAX) {
        return BSP_ERROR_PARAM;
    }

    const eui_instance_map_t *map = &s_eui_map[instance];
    if (map->has_pinmux == 0U) {
        return BSP_ERROR_UNSUPPORTED;
    }

    eui_ctrl_config_t config;
    uint32_t clk_freq;

    clock_add_to_group(map->clock_name, BOARD_RUNNING_CORE & 0x1);
    clk_freq = clock_get_frequency(map->clock_name);
    eui_get_default_ctrl_config(map->base, &config);
    eui_config_ctrl(map->base, clk_freq, &config);
    eui_set_enable(map->base, true);
    return BSP_OK;
}

int32_t bsp_eui_enable(bsp_eui_instance_t instance, bsp_state_t state)
{
    if (instance >= BSP_EUI_MAX) {
        return BSP_ERROR_PARAM;
    }

    if (s_eui_map[instance].has_pinmux == 0U) {
        return BSP_ERROR_UNSUPPORTED;
    }

    eui_set_enable(s_eui_map[instance].base, state == BSP_ENABLE);
    return BSP_OK;
}

int32_t bsp_eui_get_irq_status(bsp_eui_instance_t instance, uint32_t *status_out)
{
    if ((instance >= BSP_EUI_MAX) || (status_out == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    *status_out = eui_get_irq_status(s_eui_map[instance].base);
    return BSP_OK;
}

/**
 * @file bsp_ewdg_hpm.cpp
 * @brief HPM platform implementation for BSP EWDG interface
 */

#include "bsp_ewdg.h"
#include "board.h"
#include <string.h>

extern "C" {
#include "hpm_clock_drv.h"
#include "hpm_ewdg_drv.h"
}

typedef struct {
    EWDG_Type *base;
    clock_name_t clock_name;
    uint32_t features;
    bool initialized;
} ewdg_instance_map_t;

#define BSP_EWDG_MAP_ITEM(name, base_addr, clk_name, features) { base_addr, clk_name, features, false },
static ewdg_instance_map_t s_ewdg_map[BSP_EWDG_MAX] = {
    BSP_EWDG_LIST(BSP_EWDG_MAP_ITEM)
};
#undef BSP_EWDG_MAP_ITEM

int32_t bsp_ewdg_init(bsp_ewdg_instance_t instance, uint32_t timeout_ms)
{
    if ((instance >= BSP_EWDG_MAX) || (timeout_ms == 0U)) {
        return BSP_ERROR_PARAM;
    }

    ewdg_instance_map_t *map = &s_ewdg_map[instance];
    if ((map->features & BSP_EWDG_FEATURE_INIT) == 0U) {
        return BSP_ERROR_UNSUPPORTED;
    }
    ewdg_config_t config;

    memset(&config, 0, sizeof(config));
    clock_add_to_group(map->clock_name, BOARD_RUNNING_CORE & 0x1);
    ewdg_get_default_config(map->base, &config);
    config.enable_watchdog = true;
    config.ctrl_config.use_lowlevel_timeout = false;
    config.ctrl_config.cnt_clk_sel = ewdg_cnt_clk_src_ext_osc_clk;
    config.ctrl_config.timeout_reset_us = timeout_ms * 1000UL;
    config.int_rst_config.enable_timeout_reset = false;
    config.cnt_src_freq = 32768UL;

    if (ewdg_init(map->base, &config) != status_success) {
        return BSP_ERROR;
    }

    map->initialized = true;
    return BSP_OK;
}

int32_t bsp_ewdg_refresh(bsp_ewdg_instance_t instance)
{
    if (instance >= BSP_EWDG_MAX) {
        return BSP_ERROR_PARAM;
    }

    if ((s_ewdg_map[instance].features & BSP_EWDG_FEATURE_REFRESH) == 0U) {
        return BSP_ERROR_UNSUPPORTED;
    }

    if (!s_ewdg_map[instance].initialized) {
        return BSP_ERROR;
    }

    return (ewdg_refresh(s_ewdg_map[instance].base) == status_success) ? BSP_OK : BSP_ERROR;
}

int32_t bsp_ewdg_deinit(bsp_ewdg_instance_t instance)
{
    if (instance >= BSP_EWDG_MAX) {
        return BSP_ERROR_PARAM;
    }

    if ((s_ewdg_map[instance].features & BSP_EWDG_FEATURE_DEINIT) == 0U) {
        return BSP_ERROR_UNSUPPORTED;
    }

    ewdg_disable(s_ewdg_map[instance].base);
    s_ewdg_map[instance].initialized = false;
    return BSP_OK;
}

/**
 * @file bsp_tsns_hpm.cpp
 * @brief HPM platform implementation for BSP TSNS interface
 */

#include "bsp_tsns.h"
#include "board.h"

extern "C" {
#include "hpm_clock_drv.h"
#include "hpm_tsns_drv.h"
}

typedef struct {
    TSNS_Type *base;
    clock_name_t clock_name;
    bool initialized;
} tsns_instance_map_t;

#define BSP_TSNS_MAP_ITEM(name, base_addr, clk_name) { base_addr, clk_name, false },
static tsns_instance_map_t s_tsns_map[BSP_TSNS_MAX] = {
    BSP_TSNS_LIST(BSP_TSNS_MAP_ITEM)
};
#undef BSP_TSNS_MAP_ITEM

int32_t bsp_tsns_init(bsp_tsns_instance_t instance)
{
    if (instance >= BSP_TSNS_MAX) {
        return BSP_ERROR_PARAM;
    }

    tsns_instance_map_t *map = &s_tsns_map[instance];
    clock_add_to_group(map->clock_name, BOARD_RUNNING_CORE & 0x1);
    tsns_enable_continuous_mode(map->base);
    tsns_enable(map->base);
    map->initialized = true;
    return BSP_OK;
}

int32_t bsp_tsns_read_celsius(bsp_tsns_instance_t instance, float *temperature)
{
    if ((instance >= BSP_TSNS_MAX) || (temperature == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    if (!s_tsns_map[instance].initialized) {
        int32_t status = bsp_tsns_init(instance);
        if (status != BSP_OK) {
            return status;
        }
    }

    *temperature = tsns_get_current_temp(s_tsns_map[instance].base);
    return BSP_OK;
}

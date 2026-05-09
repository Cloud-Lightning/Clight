/**
 * @file bsp_mchtmr_hpm.cpp
 * @brief HPM platform implementation for BSP MCHTMR interface
 */

#include "bsp_mchtmr.h"
#include "board.h"

extern "C" {
#include "hpm_clock_drv.h"
#include "hpm_mchtmr_drv.h"
}

typedef struct {
    MCHTMR_Type *base;
    clock_name_t clock_name;
} mchtmr_instance_map_t;

#define BSP_MCHTMR_MAP_ITEM(name, base_addr, clk_name) { base_addr, clk_name },
static const mchtmr_instance_map_t s_mchtmr_map[BSP_MCHTMR_MAX] = {
    BSP_MCHTMR_LIST(BSP_MCHTMR_MAP_ITEM)
};
#undef BSP_MCHTMR_MAP_ITEM

int32_t bsp_mchtmr_get_count(bsp_mchtmr_instance_t instance, uint64_t *count)
{
    if ((instance >= BSP_MCHTMR_MAX) || (count == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    clock_add_to_group(s_mchtmr_map[instance].clock_name, BOARD_RUNNING_CORE & 0x1);
    *count = mchtmr_get_count(s_mchtmr_map[instance].base);
    return BSP_OK;
}

int32_t bsp_mchtmr_delay_ticks(bsp_mchtmr_instance_t instance, uint64_t delay_ticks)
{
    if (instance >= BSP_MCHTMR_MAX) {
        return BSP_ERROR_PARAM;
    }

    clock_add_to_group(s_mchtmr_map[instance].clock_name, BOARD_RUNNING_CORE & 0x1);
    mchtmr_delay(s_mchtmr_map[instance].base, delay_ticks);
    return BSP_OK;
}

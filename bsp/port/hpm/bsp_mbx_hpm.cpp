/**
 * @file bsp_mbx_hpm.cpp
 * @brief HPM platform implementation for BSP MBX interface
 */

#include "bsp_mbx.h"
#include "board.h"

extern "C" {
#include "hpm_clock_drv.h"
#include "hpm_mbx_drv.h"
}

typedef struct {
    MBX_Type *base;
    clock_name_t clock_name;
} mbx_instance_map_t;

#define BSP_MBX_MAP_ITEM(name, base_addr, clk_name) { base_addr, clk_name },
static const mbx_instance_map_t s_mbx_map[BSP_MBX_MAX] = {
    BSP_MBX_LIST(BSP_MBX_MAP_ITEM)
};
#undef BSP_MBX_MAP_ITEM

static int32_t mbx_status_to_bsp(hpm_stat_t status)
{
    if (status == status_success) {
        return BSP_OK;
    }

    if (status == status_mbx_not_available) {
        return BSP_ERROR_BUSY;
    }

    return BSP_ERROR;
}

int32_t bsp_mbx_init(bsp_mbx_instance_t instance)
{
    if (instance >= BSP_MBX_MAX) {
        return BSP_ERROR_PARAM;
    }

    const mbx_instance_map_t *map = &s_mbx_map[instance];

    clock_add_to_group(map->clock_name, BOARD_RUNNING_CORE & 0x1);
    mbx_init(map->base);
    return BSP_OK;
}

int32_t bsp_mbx_try_send(bsp_mbx_instance_t instance, uint32_t data)
{
    if (instance >= BSP_MBX_MAX) {
        return BSP_ERROR_PARAM;
    }

    return mbx_status_to_bsp(mbx_send_message(s_mbx_map[instance].base, data));
}

int32_t bsp_mbx_try_receive(bsp_mbx_instance_t instance, uint32_t *data_out)
{
    if ((instance >= BSP_MBX_MAX) || (data_out == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    return mbx_status_to_bsp(mbx_retrieve_message(s_mbx_map[instance].base, data_out));
}

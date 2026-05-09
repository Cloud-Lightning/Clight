/**
 * @file bsp_crc_hpm.cpp
 * @brief HPM platform implementation for BSP CRC interface
 */

#include "bsp_crc.h"
#include "board.h"
#include <string.h>

extern "C" {
#include "hpm_clock_drv.h"
#include "hpm_crc_drv.h"
}

typedef struct {
    CRC_Type *base;
    clock_name_t clock_name;
    uint32_t features;
} crc_instance_map_t;

#define BSP_CRC_MAP_ITEM(name, base_addr, clk_name, features) { base_addr, clk_name, features },
static const crc_instance_map_t s_crc_map[BSP_CRC_MAX] = {
    BSP_CRC_LIST(BSP_CRC_MAP_ITEM)
};
#undef BSP_CRC_MAP_ITEM

int32_t bsp_crc32_calculate(bsp_crc_instance_t instance, const uint8_t *data, uint32_t len, uint32_t *crc_out)
{
    if ((instance >= BSP_CRC_MAX) || (data == nullptr) || (len == 0U) || (crc_out == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    crc_channel_config_t config;
    const crc_instance_map_t *map = &s_crc_map[instance];
    if ((map->features & BSP_CRC_FEATURE_CRC32) == 0U) {
        return BSP_ERROR_UNSUPPORTED;
    }

    memset(&config, 0, sizeof(config));
    clock_add_to_group(map->clock_name, BOARD_RUNNING_CORE & 0x1);
    config.preset = crc_preset_crc32;
    config.in_byte_order = crc_in_byte_order_lsb;
    if (crc_setup_channel_config(map->base, 0U, &config) != status_success) {
        return BSP_ERROR;
    }

    crc_calc_block_bytes(map->base, 0U, const_cast<uint8_t *>(data), len);
    *crc_out = crc_get_result(map->base, 0U);
    return BSP_OK;
}

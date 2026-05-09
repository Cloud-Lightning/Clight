/**
 * @file bsp_qeiv2_hpm.cpp
 * @brief HPM platform implementation for BSP QEIV2 interface
 */

#include "bsp_qeiv2.h"
#include "board.h"
#include <string.h>

extern "C" {
#include "hpm_clock_drv.h"
#include "hpm_qeiv2_drv.h"
}

typedef struct {
    QEIV2_Type *base;
    uint32_t irq;
    clock_name_t clock_name;
    uint32_t phase_count_per_rev;
    bool has_pinmux;
    bool initialized;
} qeiv2_instance_map_t;

#define BSP_QEIV2_MAP_ITEM(name, base_addr, irq_num, clk_name, phase_count, has_pinmux) \
    { base_addr, irq_num, clk_name, phase_count, (has_pinmux) != 0U, false },
static qeiv2_instance_map_t s_qeiv2_map[BSP_QEIV2_MAX] = {
    BSP_QEIV2_LIST(BSP_QEIV2_MAP_ITEM)
};
#undef BSP_QEIV2_MAP_ITEM

int32_t bsp_qeiv2_init(bsp_qeiv2_instance_t qei)
{
    if (qei >= BSP_QEIV2_MAX) {
        return BSP_ERROR_PARAM;
    }

    qeiv2_instance_map_t *map = &s_qeiv2_map[qei];
    qeiv2_mode_config_t mode_config;

    if (!map->has_pinmux) {
        return BSP_ERROR_UNSUPPORTED;
    }

    memset(&mode_config, 0, sizeof(mode_config));
    clock_add_to_group(map->clock_name, BOARD_RUNNING_CORE & 0x1);
    init_qeiv2_abz_pins(map->base);

    mode_config.work_mode = qeiv2_work_mode_abz;
    mode_config.spd_tmr_content_sel = qeiv2_spd_tmr_as_spd_tm;
    mode_config.z_count_inc_mode = qeiv2_z_count_inc_on_phase_count_max;
    mode_config.phcnt_max = map->phase_count_per_rev;
    mode_config.z_cali_enable = false;
    mode_config.z_cali_ignore_ab = false;
    mode_config.phcnt_idx = 0U;
    qeiv2_config_mode(map->base, &mode_config);
    qeiv2_set_phase_cnt(map->base, 0U);
    map->initialized = true;
    return BSP_OK;
}

int32_t bsp_qeiv2_get_position(bsp_qeiv2_instance_t qei, uint32_t *position)
{
    if ((qei >= BSP_QEIV2_MAX) || (position == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    if (!s_qeiv2_map[qei].initialized) {
        int32_t status = bsp_qeiv2_init(qei);
        if (status != BSP_OK) {
            return status;
        }
    }

    *position = qeiv2_get_postion(s_qeiv2_map[qei].base);
    return BSP_OK;
}

int32_t bsp_qeiv2_get_angle(bsp_qeiv2_instance_t qei, uint32_t *angle)
{
    if ((qei >= BSP_QEIV2_MAX) || (angle == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    if (!s_qeiv2_map[qei].initialized) {
        int32_t status = bsp_qeiv2_init(qei);
        if (status != BSP_OK) {
            return status;
        }
    }

    *angle = qeiv2_get_angle(s_qeiv2_map[qei].base);
    return BSP_OK;
}

int32_t bsp_qeiv2_get_phase_count(bsp_qeiv2_instance_t qei, uint32_t *phase_count)
{
    if ((qei >= BSP_QEIV2_MAX) || (phase_count == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    if (!s_qeiv2_map[qei].initialized) {
        int32_t status = bsp_qeiv2_init(qei);
        if (status != BSP_OK) {
            return status;
        }
    }

    *phase_count = qeiv2_get_phase_cnt(s_qeiv2_map[qei].base);
    return BSP_OK;
}

int32_t bsp_qeiv2_get_speed(bsp_qeiv2_instance_t qei, uint32_t *speed)
{
    if ((qei >= BSP_QEIV2_MAX) || (speed == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    if (!s_qeiv2_map[qei].initialized) {
        int32_t status = bsp_qeiv2_init(qei);
        if (status != BSP_OK) {
            return status;
        }
    }

    *speed = qeiv2_get_count_on_read_event(s_qeiv2_map[qei].base, qeiv2_counter_type_speed);
    return BSP_OK;
}

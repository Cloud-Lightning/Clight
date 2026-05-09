/**
 * @file bsp_synt_hpm.cpp
 * @brief HPM platform implementation for BSP SYNT interface
 */

#include "bsp_synt.h"

extern "C" {
#include "hpm_synt_drv.h"
}

typedef struct {
    SYNT_Type *base;
} synt_instance_map_t;

#define BSP_SYNT_MAP_ITEM(name, base_addr) { base_addr },
static const synt_instance_map_t s_synt_map[BSP_SYNT_MAX] = {
    BSP_SYNT_LIST(BSP_SYNT_MAP_ITEM)
};
#undef BSP_SYNT_MAP_ITEM

int32_t bsp_synt_init(bsp_synt_instance_t instance, uint32_t reload_count)
{
    if (instance >= BSP_SYNT_MAX) {
        return BSP_ERROR_PARAM;
    }

    SYNT_Type *base = s_synt_map[instance].base;

    synt_enable_counter(base, false);
    synt_reset_counter(base);
    synt_set_reload(base, reload_count);
    return BSP_OK;
}

int32_t bsp_synt_start(bsp_synt_instance_t instance)
{
    if (instance >= BSP_SYNT_MAX) {
        return BSP_ERROR_PARAM;
    }

    synt_enable_counter(s_synt_map[instance].base, true);
    return BSP_OK;
}

int32_t bsp_synt_stop(bsp_synt_instance_t instance)
{
    if (instance >= BSP_SYNT_MAX) {
        return BSP_ERROR_PARAM;
    }

    synt_enable_counter(s_synt_map[instance].base, false);
    return BSP_OK;
}

int32_t bsp_synt_get_count(bsp_synt_instance_t instance, uint32_t *count_out)
{
    if ((instance >= BSP_SYNT_MAX) || (count_out == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    *count_out = synt_get_current_count(s_synt_map[instance].base);
    return BSP_OK;
}

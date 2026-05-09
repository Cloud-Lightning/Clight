/**
 * @file bsp_trgm_hpm.cpp
 * @brief HPM platform implementation for BSP TRGM interface
 */

#include "bsp_trgm.h"

extern "C" {
#include "hpm_trgm_drv.h"
}

typedef struct {
    TRGM_Type *base;
} trgm_instance_map_t;

#define BSP_TRGM_MAP_ITEM(name, base_addr) { base_addr },
static trgm_instance_map_t s_trgm_map[BSP_TRGM_MAX] = {
    BSP_TRGM_LIST(BSP_TRGM_MAP_ITEM)
};
#undef BSP_TRGM_MAP_ITEM

static trgm_output_type_t to_hpm_output_type(bsp_trgm_output_type_t type)
{
    switch (type) {
    case BSP_TRGM_OUTPUT_PULSE_FALLING:
        return trgm_output_pulse_at_input_falling_edge;
    case BSP_TRGM_OUTPUT_PULSE_RISING:
        return trgm_output_pulse_at_input_rising_edge;
    case BSP_TRGM_OUTPUT_PULSE_BOTH:
        return trgm_output_pulse_at_input_both_edge;
    case BSP_TRGM_OUTPUT_SAME_AS_INPUT:
    default:
        return trgm_output_same_as_input;
    }
}

int32_t bsp_trgm_route_output(bsp_trgm_instance_t instance, const bsp_trgm_output_route_t *route)
{
    if ((instance >= BSP_TRGM_MAX) || (route == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    trgm_output_t cfg = {};
    cfg.invert = (route->invert == BSP_ENABLE);
    cfg.type = to_hpm_output_type(route->type);
    cfg.input = static_cast<uint8_t>(route->input);
    trgm_output_config(s_trgm_map[instance].base, static_cast<uint8_t>(route->output), &cfg);
    return BSP_OK;
}

int32_t bsp_trgm_dma_request(bsp_trgm_instance_t instance, uint8_t dma_output, uint8_t dma_source)
{
    if (instance >= BSP_TRGM_MAX) {
        return BSP_ERROR_PARAM;
    }

    trgm_dma_request_config(s_trgm_map[instance].base, dma_output, dma_source);
    return BSP_OK;
}

int32_t bsp_trgm_io_output(bsp_trgm_instance_t instance, uint32_t mask, bsp_state_t state)
{
    if (instance >= BSP_TRGM_MAX) {
        return BSP_ERROR_PARAM;
    }

    if (state == BSP_ENABLE) {
        trgm_enable_io_output(s_trgm_map[instance].base, mask);
    } else {
        trgm_disable_io_output(s_trgm_map[instance].base, mask);
    }
    return BSP_OK;
}

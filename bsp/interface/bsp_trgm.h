/**
 * @file bsp_trgm.h
 * @brief BSP TRGM interface
 */

#ifndef BSP_TRGM_H
#define BSP_TRGM_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_TRGM_LIST
#if defined(BSP_TRGM_MAIN_BASE)
#define BSP_TRGM_LIST(X) \
    X(BSP_TRGM_MAIN, BSP_TRGM_MAIN_BASE)
#else
#define BSP_TRGM_LIST(X)
#endif
#endif

typedef enum {
#define BSP_TRGM_ENUM_ITEM(name, base) name,
    BSP_TRGM_LIST(BSP_TRGM_ENUM_ITEM)
#undef BSP_TRGM_ENUM_ITEM
    BSP_TRGM_MAX
} bsp_trgm_instance_t;

typedef enum {
    BSP_TRGM_OUTPUT_SAME_AS_INPUT = 0,
    BSP_TRGM_OUTPUT_PULSE_FALLING = 1,
    BSP_TRGM_OUTPUT_PULSE_RISING = 2,
    BSP_TRGM_OUTPUT_PULSE_BOTH = 3,
} bsp_trgm_output_type_t;

typedef struct {
    uint32_t input;
    uint32_t output;
    bsp_state_t invert;
    bsp_trgm_output_type_t type;
} bsp_trgm_output_route_t;

int32_t bsp_trgm_route_output(bsp_trgm_instance_t instance, const bsp_trgm_output_route_t *route);
int32_t bsp_trgm_dma_request(bsp_trgm_instance_t instance, uint8_t dma_output, uint8_t dma_source);
int32_t bsp_trgm_io_output(bsp_trgm_instance_t instance, uint32_t mask, bsp_state_t state);

#ifdef __cplusplus
}
#endif

#endif /* BSP_TRGM_H */

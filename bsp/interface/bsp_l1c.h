/**
 * @file bsp_l1c.h
 * @brief BSP L1C interface
 */

#ifndef BSP_L1C_H
#define BSP_L1C_H

#include "bsp_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BSP_L1C_MAIN = 0,
    BSP_L1C_MAX
} bsp_l1c_instance_t;

int32_t bsp_l1c_get_status(bsp_l1c_instance_t instance, bool *ic_enabled, bool *dc_enabled);
int32_t bsp_l1c_invalidate_dcache_all(bsp_l1c_instance_t instance);

#ifdef __cplusplus
}
#endif

#endif /* BSP_L1C_H */

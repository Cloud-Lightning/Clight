/**
 * @file bsp_sdm.h
 * @brief BSP SDM interface
 */

#ifndef BSP_SDM_H
#define BSP_SDM_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_SDM_LIST
#define BSP_SDM_LIST(X) \
    X(BSP_SDM_MAIN, BSP_SDM_MAIN_BASE, BSP_SDM_MAIN_CLK_NAME, BSP_SDM_MAIN_HAS_PINMUX)
#endif

typedef enum {
#define BSP_SDM_ENUM_ITEM(name, base, clk, has_pinmux) name,
    BSP_SDM_LIST(BSP_SDM_ENUM_ITEM)
#undef BSP_SDM_ENUM_ITEM
    BSP_SDM_MAX
} bsp_sdm_instance_t;

int32_t bsp_sdm_init(bsp_sdm_instance_t instance, uint8_t channel);
int32_t bsp_sdm_get_status(bsp_sdm_instance_t instance, uint32_t *status_out);

#ifdef __cplusplus
}
#endif

#endif /* BSP_SDM_H */

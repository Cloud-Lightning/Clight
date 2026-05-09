/**
 * @file bsp_ewdg.h
 * @brief BSP EWDG interface
 */

#ifndef BSP_EWDG_H
#define BSP_EWDG_H

#include "bsp_common.h"

#define BSP_EWDG_FEATURE_INIT    (1U << 0)
#define BSP_EWDG_FEATURE_REFRESH (1U << 1)
#define BSP_EWDG_FEATURE_DEINIT  (1U << 2)

#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_EWDG_LIST
#define BSP_EWDG_LIST(X) \
    X(BSP_EWDG_MAIN, BSP_EWDG_MAIN_BASE, BSP_EWDG_MAIN_CLK_NAME, BSP_EWDG_FEATURE_INIT | BSP_EWDG_FEATURE_REFRESH | BSP_EWDG_FEATURE_DEINIT)
#endif

typedef enum {
#define BSP_EWDG_ENUM_ITEM(name, base, clk, features) name,
    BSP_EWDG_LIST(BSP_EWDG_ENUM_ITEM)
#undef BSP_EWDG_ENUM_ITEM
    BSP_EWDG_MAX
} bsp_ewdg_instance_t;

int32_t bsp_ewdg_init(bsp_ewdg_instance_t instance, uint32_t timeout_ms);
int32_t bsp_ewdg_refresh(bsp_ewdg_instance_t instance);
int32_t bsp_ewdg_deinit(bsp_ewdg_instance_t instance);

#ifdef __cplusplus
}
#endif

#endif /* BSP_EWDG_H */

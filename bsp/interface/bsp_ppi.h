/**
 * @file bsp_ppi.h
 * @brief BSP PPI interface
 */

#ifndef BSP_PPI_H
#define BSP_PPI_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_PPI_LIST
#define BSP_PPI_LIST(X) \
    X(BSP_PPI_MAIN, BSP_PPI_MAIN_BASE, BSP_PPI_MAIN_CLK_NAME, BSP_PPI_MAIN_HAS_PINMUX)
#endif

typedef enum {
#define BSP_PPI_ENUM_ITEM(name, base, clk, has_pinmux) name,
    BSP_PPI_LIST(BSP_PPI_ENUM_ITEM)
#undef BSP_PPI_ENUM_ITEM
    BSP_PPI_MAX
} bsp_ppi_instance_t;

int32_t bsp_ppi_init(bsp_ppi_instance_t instance);
int32_t bsp_ppi_config_timeout(bsp_ppi_instance_t instance, uint16_t timeout_count, bsp_state_t state);

#ifdef __cplusplus
}
#endif

#endif /* BSP_PPI_H */

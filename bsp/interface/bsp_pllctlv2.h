/**
 * @file bsp_pllctlv2.h
 * @brief BSP PLLCTLV2 interface
 */

#ifndef BSP_PLLCTLV2_H
#define BSP_PLLCTLV2_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_PLLCTLV2_LIST
#define BSP_PLLCTLV2_LIST(X) \
    X(BSP_PLLCTLV2_MAIN, BSP_PLLCTLV2_MAIN_BASE)
#endif

typedef enum {
#define BSP_PLLCTLV2_ENUM_ITEM(name, base) name,
    BSP_PLLCTLV2_LIST(BSP_PLLCTLV2_ENUM_ITEM)
#undef BSP_PLLCTLV2_ENUM_ITEM
    BSP_PLLCTLV2_MAX
} bsp_pllctlv2_instance_t;

int32_t bsp_pllctlv2_get_pll_freq(bsp_pllctlv2_instance_t instance, uint8_t pll_index, uint32_t *freq_out);
int32_t bsp_pllctlv2_is_pll_stable(bsp_pllctlv2_instance_t instance, uint8_t pll_index, bool *stable_out);

#ifdef __cplusplus
}
#endif

#endif /* BSP_PLLCTLV2_H */

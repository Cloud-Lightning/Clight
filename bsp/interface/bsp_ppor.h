/**
 * @file bsp_ppor.h
 * @brief BSP PPOR interface
 */

#ifndef BSP_PPOR_H
#define BSP_PPOR_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_PPOR_LIST
#define BSP_PPOR_LIST(X) \
    X(BSP_PPOR_MAIN, BSP_PPOR_MAIN_BASE)
#endif

typedef enum {
#define BSP_PPOR_ENUM_ITEM(name, base) name,
    BSP_PPOR_LIST(BSP_PPOR_ENUM_ITEM)
#undef BSP_PPOR_ENUM_ITEM
    BSP_PPOR_MAX
} bsp_ppor_instance_t;

int32_t bsp_ppor_get_reset_flags(bsp_ppor_instance_t instance, uint32_t *flags_out);
int32_t bsp_ppor_clear_reset_flags(bsp_ppor_instance_t instance, uint32_t mask);

#ifdef __cplusplus
}
#endif

#endif /* BSP_PPOR_H */

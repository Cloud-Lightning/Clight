/**
 * @file bsp_plic.h
 * @brief BSP PLIC interface
 */

#ifndef BSP_PLIC_H
#define BSP_PLIC_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_PLIC_LIST
#define BSP_PLIC_LIST(X) \
    X(BSP_PLIC_MAIN, BSP_PLIC_MAIN_BASE)
#endif

typedef enum {
#define BSP_PLIC_ENUM_ITEM(name, base) name,
    BSP_PLIC_LIST(BSP_PLIC_ENUM_ITEM)
#undef BSP_PLIC_ENUM_ITEM
    BSP_PLIC_MAX
} bsp_plic_instance_t;

int32_t bsp_plic_init(bsp_plic_instance_t instance);
int32_t bsp_plic_config_irq(bsp_plic_instance_t instance, uint32_t irq, uint32_t priority, bsp_state_t state);

#ifdef __cplusplus
}
#endif

#endif /* BSP_PLIC_H */

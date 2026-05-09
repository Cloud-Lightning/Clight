/**
 * @file bsp_dmav2.h
 * @brief BSP DMAV2 interface
 */

#ifndef BSP_DMAV2_H
#define BSP_DMAV2_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_DMAV2_LIST
#define BSP_DMAV2_LIST(X) \
    X(BSP_DMAV2_MAIN, BSP_DMAV2_MAIN_BASE, BSP_DMAV2_MAIN_IRQ, BSP_DMAV2_MAIN_CLK_NAME)
#endif

typedef enum {
#define BSP_DMAV2_ENUM_ITEM(name, base, irq, clk) name,
    BSP_DMAV2_LIST(BSP_DMAV2_ENUM_ITEM)
#undef BSP_DMAV2_ENUM_ITEM
    BSP_DMAV2_MAX
} bsp_dmav2_instance_t;

typedef void (*bsp_dmav2_callback_t)(int32_t status, void *arg);

int32_t bsp_dmav2_memcpy(bsp_dmav2_instance_t instance, void *dst, const void *src, uint32_t size);
int32_t bsp_dmav2_memcpy_async(bsp_dmav2_instance_t instance, void *dst, const void *src, uint32_t size);
int32_t bsp_dmav2_register_callback(bsp_dmav2_instance_t instance, bsp_dmav2_callback_t callback, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* BSP_DMAV2_H */

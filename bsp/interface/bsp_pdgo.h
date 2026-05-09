/**
 * @file bsp_pdgo.h
 * @brief BSP PDGO interface
 */

#ifndef BSP_PDGO_H
#define BSP_PDGO_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_PDGO_LIST
#define BSP_PDGO_LIST(X) \
    X(BSP_PDGO_MAIN, BSP_PDGO_MAIN_BASE)
#endif

typedef enum {
#define BSP_PDGO_ENUM_ITEM(name, base) name,
    BSP_PDGO_LIST(BSP_PDGO_ENUM_ITEM)
#undef BSP_PDGO_ENUM_ITEM
    BSP_PDGO_MAX
} bsp_pdgo_instance_t;

int32_t bsp_pdgo_init(bsp_pdgo_instance_t instance);
int32_t bsp_pdgo_get_wakeup_status(bsp_pdgo_instance_t instance, uint32_t *status_out);
int32_t bsp_pdgo_write_gpr(bsp_pdgo_instance_t instance, uint32_t index, uint32_t value);
int32_t bsp_pdgo_read_gpr(bsp_pdgo_instance_t instance, uint32_t index, uint32_t *value_out);

#ifdef __cplusplus
}
#endif

#endif /* BSP_PDGO_H */

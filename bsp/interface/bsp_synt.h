/**
 * @file bsp_synt.h
 * @brief BSP SYNT interface
 */

#ifndef BSP_SYNT_H
#define BSP_SYNT_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_SYNT_LIST
#define BSP_SYNT_LIST(X) \
    X(BSP_SYNT_MAIN, BSP_SYNT_MAIN_BASE)
#endif

typedef enum {
#define BSP_SYNT_ENUM_ITEM(name, base) name,
    BSP_SYNT_LIST(BSP_SYNT_ENUM_ITEM)
#undef BSP_SYNT_ENUM_ITEM
    BSP_SYNT_MAX
} bsp_synt_instance_t;

int32_t bsp_synt_init(bsp_synt_instance_t instance, uint32_t reload_count);
int32_t bsp_synt_start(bsp_synt_instance_t instance);
int32_t bsp_synt_stop(bsp_synt_instance_t instance);
int32_t bsp_synt_get_count(bsp_synt_instance_t instance, uint32_t *count_out);

#ifdef __cplusplus
}
#endif

#endif /* BSP_SYNT_H */

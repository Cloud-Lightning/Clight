#ifndef BSP_CORDIC_H
#define BSP_CORDIC_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_CORDIC_LIST
#define BSP_CORDIC_LIST(X) \
    X(BSP_CORDIC_MAIN, BSP_CORDIC_MAIN_BASE)
#endif

typedef enum {
#define BSP_CORDIC_ENUM_ITEM(name, base) name,
    BSP_CORDIC_LIST(BSP_CORDIC_ENUM_ITEM)
#undef BSP_CORDIC_ENUM_ITEM
    BSP_CORDIC_MAX
} bsp_cordic_instance_t;

int32_t bsp_cordic_init(bsp_cordic_instance_t instance);

#ifdef __cplusplus
}
#endif

#endif

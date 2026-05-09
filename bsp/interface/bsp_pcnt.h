#ifndef BSP_PCNT_H
#define BSP_PCNT_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_PCNT_LIST
#define BSP_PCNT_LIST(X) \
    X(BSP_PCNT_MAIN, 0U, 0U)
#endif

typedef enum {
#define BSP_PCNT_ENUM_ITEM(name, unit, has_pinmux) name,
    BSP_PCNT_LIST(BSP_PCNT_ENUM_ITEM)
#undef BSP_PCNT_ENUM_ITEM
    BSP_PCNT_MAX
} bsp_pcnt_instance_t;

int32_t bsp_pcnt_init(bsp_pcnt_instance_t instance);

#ifdef __cplusplus
}
#endif

#endif

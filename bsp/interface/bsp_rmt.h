#ifndef BSP_RMT_H
#define BSP_RMT_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_RMT_LIST
#define BSP_RMT_LIST(X) \
    X(BSP_RMT_MAIN, 0U, 0U)
#endif

typedef enum {
#define BSP_RMT_ENUM_ITEM(name, channel, has_pinmux) name,
    BSP_RMT_LIST(BSP_RMT_ENUM_ITEM)
#undef BSP_RMT_ENUM_ITEM
    BSP_RMT_MAX
} bsp_rmt_instance_t;

int32_t bsp_rmt_init(bsp_rmt_instance_t instance);

#ifdef __cplusplus
}
#endif

#endif

#ifndef BSP_TOUCH_H
#define BSP_TOUCH_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_TOUCH_LIST
#define BSP_TOUCH_LIST(X) \
    X(BSP_TOUCH_MAIN, 0U, 0U)
#endif

typedef enum {
#define BSP_TOUCH_ENUM_ITEM(name, channel, has_pinmux) name,
    BSP_TOUCH_LIST(BSP_TOUCH_ENUM_ITEM)
#undef BSP_TOUCH_ENUM_ITEM
    BSP_TOUCH_MAX
} bsp_touch_instance_t;

int32_t bsp_touch_init(bsp_touch_instance_t instance);

#ifdef __cplusplus
}
#endif

#endif

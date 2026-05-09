#ifndef BSP_MCPWM_H
#define BSP_MCPWM_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_MCPWM_LIST
#define BSP_MCPWM_LIST(X) \
    X(BSP_MCPWM_MAIN, 0U, 0U)
#endif

typedef enum {
#define BSP_MCPWM_ENUM_ITEM(name, group, has_pinmux) name,
    BSP_MCPWM_LIST(BSP_MCPWM_ENUM_ITEM)
#undef BSP_MCPWM_ENUM_ITEM
    BSP_MCPWM_MAX
} bsp_mcpwm_instance_t;

int32_t bsp_mcpwm_init(bsp_mcpwm_instance_t instance);

#ifdef __cplusplus
}
#endif

#endif

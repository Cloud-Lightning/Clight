#ifndef BSP_WWDG_H
#define BSP_WWDG_H

#include "bsp_common.h"

#define BSP_WWDG_FEATURE_INIT    (1U << 0)
#define BSP_WWDG_FEATURE_REFRESH (1U << 1)

#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_WWDG_LIST
#define BSP_WWDG_LIST(X) \
    X(BSP_WWDG_MAIN, BSP_WWDG_MAIN_BASE, BSP_WWDG_MAIN_IRQ, BSP_WWDG_FEATURE_INIT | BSP_WWDG_FEATURE_REFRESH)
#endif

typedef enum {
#define BSP_WWDG_ENUM_ITEM(name, base, irq, features) name,
    BSP_WWDG_LIST(BSP_WWDG_ENUM_ITEM)
#undef BSP_WWDG_ENUM_ITEM
    BSP_WWDG_MAX
} bsp_wwdg_instance_t;

int32_t bsp_wwdg_init(bsp_wwdg_instance_t instance, uint32_t timeout_ms);
int32_t bsp_wwdg_refresh(bsp_wwdg_instance_t instance);

#ifdef __cplusplus
}
#endif

#endif

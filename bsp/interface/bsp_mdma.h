#ifndef BSP_MDMA_H
#define BSP_MDMA_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_MDMA_LIST
#define BSP_MDMA_LIST(X) \
    X(BSP_MDMA_MAIN, BSP_MDMA_MAIN_BASE)
#endif

typedef enum {
#define BSP_MDMA_ENUM_ITEM(name, base) name,
    BSP_MDMA_LIST(BSP_MDMA_ENUM_ITEM)
#undef BSP_MDMA_ENUM_ITEM
    BSP_MDMA_MAX
} bsp_mdma_instance_t;

int32_t bsp_mdma_init(bsp_mdma_instance_t instance);

#ifdef __cplusplus
}
#endif

#endif

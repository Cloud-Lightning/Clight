#ifndef BSP_PSSI_H
#define BSP_PSSI_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_PSSI_LIST
#define BSP_PSSI_LIST(X) \
    X(BSP_PSSI, BSP_PSSI_BASE, BSP_PSSI_HAS_PINMUX)
#endif

typedef enum {
#define BSP_PSSI_ENUM_ITEM(name, base, available) name,
    BSP_PSSI_LIST(BSP_PSSI_ENUM_ITEM)
#undef BSP_PSSI_ENUM_ITEM
    BSP_PSSI_MAX
} bsp_pssi_instance_t;

int32_t bsp_pssi_init(bsp_pssi_instance_t instance);

#ifdef __cplusplus
}
#endif

#endif

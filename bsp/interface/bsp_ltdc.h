#ifndef BSP_LTDC_H
#define BSP_LTDC_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_LTDC_LIST
#define BSP_LTDC_LIST(X) \
    X(BSP_LTDC, BSP_LTDC_BASE, BSP_LTDC_HAS_PINMUX)
#endif

typedef enum {
#define BSP_LTDC_ENUM_ITEM(name, base, available) name,
    BSP_LTDC_LIST(BSP_LTDC_ENUM_ITEM)
#undef BSP_LTDC_ENUM_ITEM
    BSP_LTDC_MAX
} bsp_ltdc_instance_t;

int32_t bsp_ltdc_init(bsp_ltdc_instance_t instance);

#ifdef __cplusplus
}
#endif

#endif

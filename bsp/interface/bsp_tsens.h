#ifndef BSP_TSENS_H
#define BSP_TSENS_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_TSENS_LIST
#define BSP_TSENS_LIST(X) \
    X(BSP_TSENS_MAIN)
#endif

typedef enum {
#define BSP_TSENS_ENUM_ITEM(name) name,
    BSP_TSENS_LIST(BSP_TSENS_ENUM_ITEM)
#undef BSP_TSENS_ENUM_ITEM
    BSP_TSENS_MAX
} bsp_tsens_instance_t;

int32_t bsp_tsens_init(bsp_tsens_instance_t instance);
int32_t bsp_tsens_read_celsius(bsp_tsens_instance_t instance, float *celsius);

#ifdef __cplusplus
}
#endif

#endif

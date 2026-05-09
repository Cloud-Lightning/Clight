#ifndef BSP_SAI_H
#define BSP_SAI_H

#include "bsp_common.h"

#define BSP_SAI_FEATURE_INIT           (1U << 0)
#define BSP_SAI_FEATURE_BLOCKING_READ  (1U << 1)
#define BSP_SAI_FEATURE_BLOCKING_WRITE (1U << 2)

#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_SAI_LIST
#define BSP_SAI_LIST(X) \
    X(BSP_SAI1, BSP_SAI1_BASE, BSP_SAI1_HAS_PINMUX, 0U)
#endif

typedef enum {
#define BSP_SAI_ENUM_ITEM(name, base, available, features) name,
    BSP_SAI_LIST(BSP_SAI_ENUM_ITEM)
#undef BSP_SAI_ENUM_ITEM
    BSP_SAI_MAX
} bsp_sai_instance_t;

int32_t bsp_sai_init(bsp_sai_instance_t instance);
int32_t bsp_sai_write(bsp_sai_instance_t instance, const uint8_t *data, uint32_t len);
int32_t bsp_sai_read(bsp_sai_instance_t instance, uint8_t *data, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif

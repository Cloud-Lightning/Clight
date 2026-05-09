#ifndef BSP_RNG_H
#define BSP_RNG_H

#include "bsp_common.h"

#define BSP_RNG_FEATURE_U32 (1U << 0)

#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_RNG_LIST
#define BSP_RNG_LIST(X) \
    X(BSP_RNG_MAIN, BSP_RNG_MAIN_BASE, BSP_RNG_FEATURE_U32)
#endif

typedef enum {
#define BSP_RNG_ENUM_ITEM(name, base, features) name,
    BSP_RNG_LIST(BSP_RNG_ENUM_ITEM)
#undef BSP_RNG_ENUM_ITEM
    BSP_RNG_MAX
} bsp_rng_instance_t;

int32_t bsp_rng_init(bsp_rng_instance_t instance);
int32_t bsp_rng_get_u32(bsp_rng_instance_t instance, uint32_t *value);

#ifdef __cplusplus
}
#endif

#endif

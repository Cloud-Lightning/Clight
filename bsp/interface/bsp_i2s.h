#ifndef BSP_I2S_H
#define BSP_I2S_H

#include "bsp_common.h"

#define BSP_I2S_FEATURE_INIT           (1U << 0)
#define BSP_I2S_FEATURE_BLOCKING_READ  (1U << 1)
#define BSP_I2S_FEATURE_BLOCKING_WRITE (1U << 2)

#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_I2S_LIST
#define BSP_I2S_LIST(X) \
    X(BSP_I2S_MAIN, 0U, 0U, 0U)
#endif

typedef enum {
#define BSP_I2S_ENUM_ITEM(name, port, has_pinmux, features) name,
    BSP_I2S_LIST(BSP_I2S_ENUM_ITEM)
#undef BSP_I2S_ENUM_ITEM
    BSP_I2S_MAX
} bsp_i2s_instance_t;

int32_t bsp_i2s_init(bsp_i2s_instance_t instance);
int32_t bsp_i2s_write(bsp_i2s_instance_t instance, const uint8_t *data, uint32_t len);
int32_t bsp_i2s_read(bsp_i2s_instance_t instance, uint8_t *data, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif

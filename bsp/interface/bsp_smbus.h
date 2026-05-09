#ifndef BSP_SMBUS_H
#define BSP_SMBUS_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_SMBUS_LIST
#define BSP_SMBUS_LIST(X) \
    X(BSP_SMBUS_I2C1, BSP_SMBUS_I2C1_BASE, BSP_SMBUS_I2C1_HAS_PINMUX)
#endif

typedef enum {
#define BSP_SMBUS_ENUM_ITEM(name, base, available) name,
    BSP_SMBUS_LIST(BSP_SMBUS_ENUM_ITEM)
#undef BSP_SMBUS_ENUM_ITEM
    BSP_SMBUS_MAX
} bsp_smbus_instance_t;

int32_t bsp_smbus_init(bsp_smbus_instance_t instance);

#ifdef __cplusplus
}
#endif

#endif

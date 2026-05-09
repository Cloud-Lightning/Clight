/**
 * @file bsp_i2c.h
 * @brief BSP I2C interface
 */

#ifndef BSP_I2C_H
#define BSP_I2C_H

#include "bsp_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BSP_I2C_FEATURE_MASTER             (1U << 0)
#define BSP_I2C_FEATURE_BLOCKING           (1U << 1)
#define BSP_I2C_FEATURE_INTERRUPT          (1U << 2)
#define BSP_I2C_FEATURE_DMA                (1U << 3)
#define BSP_I2C_FEATURE_TEN_BIT_ADDRESS    (1U << 4)
#define BSP_I2C_FEATURE_FIXED_TIMING       (1U << 5)
#define BSP_I2C_FEATURE_CONFIGURABLE_CLOCK (1U << 6)

#include "board_config.h"

#ifndef BSP_I2C_LIST
#define BSP_I2C_LIST(X) \
    X(BSP_I2C_MAIN, BSP_I2C_MAIN_BASE, BSP_I2C_MAIN_IRQ, BSP_I2C_MAIN_CLK_NAME, BSP_I2C_MAIN_HAS_PINMUX, \
      BSP_I2C_FEATURE_MASTER | BSP_I2C_FEATURE_BLOCKING | BSP_I2C_FEATURE_FIXED_TIMING)
#endif

typedef enum {
#define BSP_I2C_ENUM_ITEM(name, base, irq, clk, has_pinmux, features) name,
    BSP_I2C_LIST(BSP_I2C_ENUM_ITEM)
#undef BSP_I2C_ENUM_ITEM
    BSP_I2C_MAX
} bsp_i2c_instance_t;

typedef enum {
    BSP_I2C_EVENT_WRITE = 0,
    BSP_I2C_EVENT_READ,
} bsp_i2c_event_t;

typedef void (*bsp_i2c_event_callback_t)(bsp_i2c_event_t event, int32_t status, void *arg);

int32_t bsp_i2c_init(bsp_i2c_instance_t i2c,
                     uint32_t frequency_hz,
                     bsp_state_t ten_bit_addressing,
                     bsp_transfer_mode_t transfer_mode,
                     uint8_t irq_priority);
int32_t bsp_i2c_write(bsp_i2c_instance_t i2c, uint16_t device_address, const uint8_t *data, uint32_t size);
int32_t bsp_i2c_read(bsp_i2c_instance_t i2c, uint16_t device_address, uint8_t *data, uint32_t size);
int32_t bsp_i2c_write_async(bsp_i2c_instance_t i2c, uint16_t device_address, const uint8_t *data, uint32_t size);
int32_t bsp_i2c_read_async(bsp_i2c_instance_t i2c, uint16_t device_address, uint8_t *data, uint32_t size);
int32_t bsp_i2c_register_event_callback(bsp_i2c_instance_t i2c, bsp_i2c_event_callback_t callback, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* BSP_I2C_H */

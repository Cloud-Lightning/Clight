/**
 * @file bsp_gpio.h
 * @brief BSP GPIO interface
 */

#ifndef BSP_GPIO_H
#define BSP_GPIO_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * GPIO resource list comes from board_config.h.
 * Fallback keeps compatibility with legacy board_config definitions.
 */
#ifndef BSP_GPIO_LIST
#define BSP_GPIO_LIST(X) \
    X(BSP_GPIO_LED, BSP_GPIO_LED_PORT, BSP_GPIO_LED_INDEX, BSP_GPIO_LED_PIN_INDEX) \
    X(BSP_GPIO_KEY, BSP_GPIO_KEY_PORT, BSP_GPIO_KEY_INDEX, BSP_GPIO_KEY_PIN_INDEX)
#endif

typedef enum {
#define BSP_GPIO_ENUM_ITEM(name, port, index, pin_index) name,
    BSP_GPIO_LIST(BSP_GPIO_ENUM_ITEM)
#undef BSP_GPIO_ENUM_ITEM
    BSP_GPIO_MAX
} bsp_gpio_pin_t;

typedef enum {
    BSP_GPIO_MODE_INPUT = 0,
    BSP_GPIO_MODE_OUTPUT,
    BSP_GPIO_MODE_IT_RISING,
    BSP_GPIO_MODE_IT_FALLING,
    BSP_GPIO_MODE_IT_BOTH
} bsp_gpio_mode_t;

typedef enum {
    BSP_GPIO_PULL_NONE = 0,
    BSP_GPIO_PULL_UP,
    BSP_GPIO_PULL_DOWN,
} bsp_gpio_pull_t;

typedef enum {
    BSP_GPIO_OUTPUT_PUSH_PULL = 0,
    BSP_GPIO_OUTPUT_OPEN_DRAIN,
} bsp_gpio_output_type_t;

typedef void (*bsp_gpio_irq_callback_t)(bsp_gpio_pin_t pin, void *arg);

int32_t bsp_gpio_init(bsp_gpio_pin_t pin, bsp_gpio_mode_t mode, bsp_gpio_pull_t pull, bsp_gpio_output_type_t output_type);
void bsp_gpio_write(bsp_gpio_pin_t pin, bsp_level_t level);
bsp_level_t bsp_gpio_read(bsp_gpio_pin_t pin);
void bsp_gpio_toggle(bsp_gpio_pin_t pin);
int32_t bsp_gpio_register_callback(bsp_gpio_pin_t pin, bsp_gpio_irq_callback_t callback, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* BSP_GPIO_H */

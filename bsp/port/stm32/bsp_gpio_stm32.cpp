#include "bsp_gpio.h"

#include <cstdint>

extern "C" {
#include "gpio.h"
}

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
} gpio_pin_map_t;

#define BSP_GPIO_MAP_ITEM(name, port, index, pin_index) { port, static_cast<uint16_t>(pin_index) },
static const gpio_pin_map_t s_gpio_map[BSP_GPIO_MAX] = {
    BSP_GPIO_LIST(BSP_GPIO_MAP_ITEM)
};
#undef BSP_GPIO_MAP_ITEM

static bsp_gpio_irq_callback_t s_gpio_callbacks[BSP_GPIO_MAX] = {nullptr};
static void *s_gpio_callback_args[BSP_GPIO_MAX] = {nullptr};

static void ensure_gpio_initialized()
{
    static bool initialized = false;
    if (!initialized) {
        MX_GPIO_Init();
        initialized = true;
    }
}

static int32_t init_pin(GPIO_TypeDef *port,
                        uint16_t pin,
                        bsp_gpio_mode_t mode,
                        bsp_gpio_pull_t pull,
                        bsp_gpio_output_type_t output_type)
{
    GPIO_InitTypeDef init = {};
    init.Pin = pin;
    init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    switch (pull) {
    case BSP_GPIO_PULL_UP:
        init.Pull = GPIO_PULLUP;
        break;
    case BSP_GPIO_PULL_DOWN:
        init.Pull = GPIO_PULLDOWN;
        break;
    case BSP_GPIO_PULL_NONE:
    default:
        init.Pull = GPIO_NOPULL;
        break;
    }

    switch (mode) {
    case BSP_GPIO_MODE_INPUT:
        init.Mode = GPIO_MODE_INPUT;
        break;
    case BSP_GPIO_MODE_OUTPUT:
        init.Mode = (output_type == BSP_GPIO_OUTPUT_OPEN_DRAIN) ? GPIO_MODE_OUTPUT_OD : GPIO_MODE_OUTPUT_PP;
        break;
    case BSP_GPIO_MODE_IT_RISING:
        init.Mode = GPIO_MODE_IT_RISING;
        break;
    case BSP_GPIO_MODE_IT_FALLING:
        init.Mode = GPIO_MODE_IT_FALLING;
        break;
    case BSP_GPIO_MODE_IT_BOTH:
        init.Mode = GPIO_MODE_IT_RISING_FALLING;
        break;
    default:
        return BSP_ERROR_PARAM;
    }

    HAL_GPIO_Init(port, &init);
    return BSP_OK;
}

int32_t bsp_gpio_init(bsp_gpio_pin_t pin, bsp_gpio_mode_t mode, bsp_gpio_pull_t pull, bsp_gpio_output_type_t output_type)
{
    if (pin >= BSP_GPIO_MAX) {
        return BSP_ERROR_PARAM;
    }

    ensure_gpio_initialized();
    return init_pin(s_gpio_map[pin].port, s_gpio_map[pin].pin, mode, pull, output_type);
}

void bsp_gpio_write(bsp_gpio_pin_t pin, bsp_level_t level)
{
    if (pin >= BSP_GPIO_MAX) {
        return;
    }

    ensure_gpio_initialized();
    HAL_GPIO_WritePin(s_gpio_map[pin].port, s_gpio_map[pin].pin, (level == BSP_LEVEL_HIGH) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

bsp_level_t bsp_gpio_read(bsp_gpio_pin_t pin)
{
    if (pin >= BSP_GPIO_MAX) {
        return BSP_LEVEL_LOW;
    }

    ensure_gpio_initialized();
    return (HAL_GPIO_ReadPin(s_gpio_map[pin].port, s_gpio_map[pin].pin) == GPIO_PIN_SET) ? BSP_LEVEL_HIGH : BSP_LEVEL_LOW;
}

void bsp_gpio_toggle(bsp_gpio_pin_t pin)
{
    if (pin >= BSP_GPIO_MAX) {
        return;
    }

    ensure_gpio_initialized();
    HAL_GPIO_TogglePin(s_gpio_map[pin].port, s_gpio_map[pin].pin);
}

int32_t bsp_gpio_register_callback(bsp_gpio_pin_t pin, bsp_gpio_irq_callback_t callback, void *arg)
{
    if (pin >= BSP_GPIO_MAX) {
        return BSP_ERROR_PARAM;
    }

    s_gpio_callbacks[pin] = callback;
    s_gpio_callback_args[pin] = arg;
    return BSP_OK;
}

extern "C" void HAL_GPIO_EXTI_Callback(uint16_t gpio_pin)
{
    for (std::uint32_t index = 0; index < BSP_GPIO_MAX; ++index) {
        if ((s_gpio_map[index].pin == gpio_pin) && (s_gpio_callbacks[index] != nullptr)) {
            s_gpio_callbacks[index](static_cast<bsp_gpio_pin_t>(index), s_gpio_callback_args[index]);
        }
    }
}

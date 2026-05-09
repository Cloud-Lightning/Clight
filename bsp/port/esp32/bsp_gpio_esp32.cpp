#include "bsp_gpio.h"

#include <cstdint>

#include "driver/gpio.h"

typedef struct {
    gpio_num_t pin;
    bool initialized;
    bsp_gpio_mode_t mode;
} gpio_instance_map_t;

#define BSP_GPIO_MAP_ITEM(name, port, index, pin_index) \
    { static_cast<gpio_num_t>(pin_index), false, BSP_GPIO_MODE_INPUT },
static gpio_instance_map_t s_gpio_map[BSP_GPIO_MAX] = {
    BSP_GPIO_LIST(BSP_GPIO_MAP_ITEM)
};
#undef BSP_GPIO_MAP_ITEM

static bsp_gpio_irq_callback_t s_gpio_callbacks[BSP_GPIO_MAX] = {nullptr};
static void *s_gpio_callback_args[BSP_GPIO_MAX] = {nullptr};
static bool s_gpio_isr_service_installed = false;

static bool gpio_mode_is_interrupt(bsp_gpio_mode_t mode)
{
    return (mode == BSP_GPIO_MODE_IT_RISING) || (mode == BSP_GPIO_MODE_IT_FALLING) || (mode == BSP_GPIO_MODE_IT_BOTH);
}

static void ensure_gpio_isr_service()
{
    if (s_gpio_isr_service_installed) {
        return;
    }

    const auto status = gpio_install_isr_service(0);
    if ((status == ESP_OK) || (status == ESP_ERR_INVALID_STATE)) {
        s_gpio_isr_service_installed = true;
    }
}

static void gpio_apply_isr_binding(bsp_gpio_pin_t pin)
{
    if (!s_gpio_isr_service_installed) {
        return;
    }

    const auto gpio_pin = s_gpio_map[pin].pin;
    (void) gpio_isr_handler_remove(gpio_pin);
    if (gpio_mode_is_interrupt(s_gpio_map[pin].mode)) {
        (void) gpio_isr_handler_add(
            gpio_pin,
            [](void *arg) {
                const auto index = static_cast<bsp_gpio_pin_t>(reinterpret_cast<std::uintptr_t>(arg));
                if ((index < BSP_GPIO_MAX) && (s_gpio_callbacks[index] != nullptr)) {
                    s_gpio_callbacks[index](index, s_gpio_callback_args[index]);
                }
            },
            reinterpret_cast<void *>(static_cast<std::uintptr_t>(pin)));
    }
}

int32_t bsp_gpio_init(bsp_gpio_pin_t pin, bsp_gpio_mode_t mode, bsp_gpio_pull_t pull, bsp_gpio_output_type_t output_type)
{
    if (pin >= BSP_GPIO_MAX) {
        return BSP_ERROR_PARAM;
    }

    (void)output_type;
    gpio_config_t config = {};
    config.pin_bit_mask = (1ULL << static_cast<unsigned>(s_gpio_map[pin].pin));
    config.pull_up_en = GPIO_PULLUP_DISABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;
    if (pull == BSP_GPIO_PULL_UP) {
        config.pull_up_en = GPIO_PULLUP_ENABLE;
    } else if (pull == BSP_GPIO_PULL_DOWN) {
        config.pull_down_en = GPIO_PULLDOWN_ENABLE;
    }

    switch (mode) {
    case BSP_GPIO_MODE_INPUT:
        config.mode = GPIO_MODE_INPUT;
        break;
    case BSP_GPIO_MODE_OUTPUT:
        config.mode = GPIO_MODE_OUTPUT;
        break;
    case BSP_GPIO_MODE_IT_RISING:
        config.mode = GPIO_MODE_INPUT;
        config.intr_type = GPIO_INTR_POSEDGE;
        break;
    case BSP_GPIO_MODE_IT_FALLING:
        config.mode = GPIO_MODE_INPUT;
        config.intr_type = GPIO_INTR_NEGEDGE;
        break;
    case BSP_GPIO_MODE_IT_BOTH:
        config.mode = GPIO_MODE_INPUT;
        config.intr_type = GPIO_INTR_ANYEDGE;
        break;
    default:
        return BSP_ERROR_PARAM;
    }

    if (gpio_config(&config) != ESP_OK) {
        return BSP_ERROR;
    }

    s_gpio_map[pin].initialized = true;
    s_gpio_map[pin].mode = mode;
    if (gpio_mode_is_interrupt(mode)) {
        ensure_gpio_isr_service();
        gpio_apply_isr_binding(pin);
        if (!s_gpio_isr_service_installed) {
            return BSP_ERROR;
        }
    } else if (s_gpio_isr_service_installed) {
        (void) gpio_isr_handler_remove(s_gpio_map[pin].pin);
    }
    return BSP_OK;
}

void bsp_gpio_write(bsp_gpio_pin_t pin, bsp_level_t level)
{
    if (pin < BSP_GPIO_MAX) {
        gpio_set_level(s_gpio_map[pin].pin, (level == BSP_LEVEL_HIGH) ? 1 : 0);
    }
}

bsp_level_t bsp_gpio_read(bsp_gpio_pin_t pin)
{
    if (pin >= BSP_GPIO_MAX) {
        return BSP_LEVEL_LOW;
    }
    return (gpio_get_level(s_gpio_map[pin].pin) != 0) ? BSP_LEVEL_HIGH : BSP_LEVEL_LOW;
}

void bsp_gpio_toggle(bsp_gpio_pin_t pin)
{
    if (pin < BSP_GPIO_MAX) {
        const auto level = gpio_get_level(s_gpio_map[pin].pin);
        gpio_set_level(s_gpio_map[pin].pin, level == 0 ? 1 : 0);
    }
}

int32_t bsp_gpio_register_callback(bsp_gpio_pin_t pin, bsp_gpio_irq_callback_t callback, void *arg)
{
    if (pin >= BSP_GPIO_MAX) {
        return BSP_ERROR_PARAM;
    }

    s_gpio_callbacks[pin] = callback;
    s_gpio_callback_args[pin] = arg;
    if (gpio_mode_is_interrupt(s_gpio_map[pin].mode)) {
        ensure_gpio_isr_service();
        gpio_apply_isr_binding(pin);
        return s_gpio_isr_service_installed ? BSP_OK : BSP_ERROR;
    }
    return BSP_OK;
}

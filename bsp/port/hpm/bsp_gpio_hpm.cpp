/**
 * @file bsp_gpio_hpm.cpp
 * @brief HPM platform implementation for BSP GPIO interface
 */

#include "bsp_gpio.h"
#include "board_config.h"

extern "C" {
#include "board.h"
#include "hpm_gpio_drv.h"
#include "hpm_interrupt.h"
#include "hpm_ioc_regs.h"
}

typedef struct {
    GPIO_Type *port;
    uint32_t index;
    uint32_t pin_index;
    uint32_t pad;
} gpio_pin_map_t;

#define BSP_GPIO_MAP_ITEM(name, port, index, pin_index) { port, index, pin_index, name##_PIN },
static const gpio_pin_map_t gpio_map[BSP_GPIO_MAX] = {
    BSP_GPIO_LIST(BSP_GPIO_MAP_ITEM)
};
#undef BSP_GPIO_MAP_ITEM

static bsp_gpio_irq_callback_t gpio_callbacks[BSP_GPIO_MAX] = {nullptr};
static void *gpio_callback_args[BSP_GPIO_MAX] = {nullptr};

static gpio_interrupt_trigger_t to_hpm_interrupt_trigger(bsp_gpio_mode_t mode)
{
    switch (mode) {
    case BSP_GPIO_MODE_IT_RISING:
        return gpio_interrupt_trigger_edge_rising;
    case BSP_GPIO_MODE_IT_FALLING:
        return gpio_interrupt_trigger_edge_falling;
    case BSP_GPIO_MODE_IT_BOTH:
    default:
        return gpio_interrupt_trigger_edge_both;
    }
}

static int32_t to_hpm_gpio_irq(uint32_t index)
{
    switch (index) {
    case GPIO_DI_GPIOA:
        return IRQn_GPIO0_A;
    case GPIO_DI_GPIOB:
        return IRQn_GPIO0_B;
    case GPIO_DI_GPIOX:
        return IRQn_GPIO0_X;
    case GPIO_DI_GPIOY:
        return IRQn_GPIO0_Y;
    default:
        return -1;
    }
}

static void dispatch_gpio_irq(GPIO_Type *port, uint32_t index)
{
    const uint32_t flags = gpio_get_port_interrupt_flags(port, index);
    if (flags == 0U) {
        return;
    }

    for (uint32_t pin = 0U; pin < BSP_GPIO_MAX; ++pin) {
        const gpio_pin_map_t *map = &gpio_map[pin];
        const uint32_t mask = 1UL << map->pin_index;
        if ((map->port != port) || (map->index != index) || ((flags & mask) == 0U)) {
            continue;
        }

        gpio_clear_pin_interrupt_flag(map->port, map->index, map->pin_index);
        if (gpio_callbacks[pin] != nullptr) {
            gpio_callbacks[pin](static_cast<bsp_gpio_pin_t>(pin), gpio_callback_args[pin]);
        }
    }
}

static void apply_pull_and_type(const gpio_pin_map_t *map, bsp_gpio_pull_t pull, bsp_gpio_output_type_t output_type)
{
    uint32_t pad_ctl = HPM_IOC->PAD[map->pad].PAD_CTL;
    pad_ctl &= ~(IOC_PAD_PAD_CTL_PE_MASK | IOC_PAD_PAD_CTL_PS_MASK | IOC_PAD_PAD_CTL_OD_MASK);

    switch (pull) {
    case BSP_GPIO_PULL_UP:
        pad_ctl |= IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(1);
        break;
    case BSP_GPIO_PULL_DOWN:
        pad_ctl |= IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(0);
        break;
    case BSP_GPIO_PULL_NONE:
    default:
        break;
    }

    if (output_type == BSP_GPIO_OUTPUT_OPEN_DRAIN) {
        pad_ctl |= IOC_PAD_PAD_CTL_OD_SET(1);
    }

    HPM_IOC->PAD[map->pad].PAD_CTL = pad_ctl;
}

int32_t bsp_gpio_init(bsp_gpio_pin_t pin, bsp_gpio_mode_t mode, bsp_gpio_pull_t pull, bsp_gpio_output_type_t output_type)
{
    if (pin >= BSP_GPIO_MAX) {
        return BSP_ERROR_PARAM;
    }

    const gpio_pin_map_t *map = &gpio_map[pin];
    apply_pull_and_type(map, pull, output_type);

    switch (mode) {
    case BSP_GPIO_MODE_INPUT:
        gpio_set_pin_input(map->port, map->index, map->pin_index);
        break;
    case BSP_GPIO_MODE_OUTPUT:
        gpio_set_pin_output(map->port, map->index, map->pin_index);
        break;
    case BSP_GPIO_MODE_IT_RISING:
    case BSP_GPIO_MODE_IT_FALLING:
    case BSP_GPIO_MODE_IT_BOTH:
        gpio_set_pin_input(map->port, map->index, map->pin_index);
        gpio_config_pin_interrupt(map->port, map->index, map->pin_index, to_hpm_interrupt_trigger(mode));
        gpio_clear_pin_interrupt_flag(map->port, map->index, map->pin_index);
        gpio_enable_pin_interrupt(map->port, map->index, map->pin_index);
        {
            const int32_t irq = to_hpm_gpio_irq(map->index);
            if (irq < 0) {
                return BSP_ERROR_UNSUPPORTED;
            }
            intc_m_enable_irq_with_priority(static_cast<uint32_t>(irq), BSP_IRQ_PRIORITY_DEFAULT);
        }
        break;
    default:
        return BSP_ERROR_PARAM;
    }

    return BSP_OK;
}

void bsp_gpio_write(bsp_gpio_pin_t pin, bsp_level_t level)
{
    if (pin >= BSP_GPIO_MAX) {
        return;
    }

    const gpio_pin_map_t *map = &gpio_map[pin];
    gpio_write_pin(map->port, map->index, map->pin_index, (level == BSP_LEVEL_HIGH) ? 1U : 0U);
}

bsp_level_t bsp_gpio_read(bsp_gpio_pin_t pin)
{
    if (pin >= BSP_GPIO_MAX) {
        return BSP_LEVEL_LOW;
    }

    const gpio_pin_map_t *map = &gpio_map[pin];
    return gpio_read_pin(map->port, map->index, map->pin_index) ? BSP_LEVEL_HIGH : BSP_LEVEL_LOW;
}

void bsp_gpio_toggle(bsp_gpio_pin_t pin)
{
    if (pin >= BSP_GPIO_MAX) {
        return;
    }

    const gpio_pin_map_t *map = &gpio_map[pin];
    gpio_toggle_pin(map->port, map->index, map->pin_index);
}

int32_t bsp_gpio_register_callback(bsp_gpio_pin_t pin, bsp_gpio_irq_callback_t callback, void *arg)
{
    if (pin >= BSP_GPIO_MAX) {
        return BSP_ERROR_PARAM;
    }

    gpio_callbacks[pin] = callback;
    gpio_callback_args[pin] = arg;
    return BSP_OK;
}

SDK_DECLARE_EXT_ISR_M(IRQn_GPIO0_A, hpm_bsp_gpio0_a_isr)
void hpm_bsp_gpio0_a_isr(void)
{
    dispatch_gpio_irq(HPM_GPIO0, GPIO_DI_GPIOA);
}

SDK_DECLARE_EXT_ISR_M(IRQn_GPIO0_B, hpm_bsp_gpio0_b_isr)
void hpm_bsp_gpio0_b_isr(void)
{
    dispatch_gpio_irq(HPM_GPIO0, GPIO_DI_GPIOB);
}

SDK_DECLARE_EXT_ISR_M(IRQn_GPIO0_X, hpm_bsp_gpio0_x_isr)
void hpm_bsp_gpio0_x_isr(void)
{
    dispatch_gpio_irq(HPM_GPIO0, GPIO_DI_GPIOX);
}

SDK_DECLARE_EXT_ISR_M(IRQn_GPIO0_Y, hpm_bsp_gpio0_y_isr)
void hpm_bsp_gpio0_y_isr(void)
{
    dispatch_gpio_irq(HPM_GPIO0, GPIO_DI_GPIOY);
}

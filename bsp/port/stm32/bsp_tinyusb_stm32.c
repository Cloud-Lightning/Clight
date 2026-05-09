#include "TinyUsbPlatform.h"

#include "main.h"
#include "stm32h7xx_hal.h"
#include "tusb.h"

#define STM32_TINYUSB_RHPORT 0U

static uint16_t hex_nibble_to_ascii(uint8_t value)
{
    value &= 0x0FU;
    if (value < 10U) {
        return (uint16_t)('0' + value);
    }
    return (uint16_t)('A' + (value - 10U));
}

static void write_hex_word(uint16_t desc_str[], size_t *index, size_t max_chars, uint32_t value)
{
    for (int shift = 28; (shift >= 0) && (*index < max_chars); shift -= 4) {
        desc_str[*index] = hex_nibble_to_ascii((uint8_t)(value >> (uint32_t)shift));
        *index = *index + 1U;
    }
}

bool tinyusb_platform_init(uint8_t rhport)
{
    if (rhport != STM32_TINYUSB_RHPORT) {
        return false;
    }

    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_11 | GPIO_PIN_12;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF10_OTG1_HS;
    HAL_GPIO_Init(GPIOA, &gpio);

    RCC_PeriphCLKInitTypeDef periph_clk = {0};
    periph_clk.PeriphClockSelection = RCC_PERIPHCLK_USB;
    periph_clk.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;
    if (HAL_RCCEx_PeriphCLKConfig(&periph_clk) != HAL_OK) {
        return false;
    }

    HAL_PWREx_EnableUSBVoltageDetector();
    __HAL_RCC_USB_OTG_HS_CLK_ENABLE();

    tud_configure_dwc2_t cfg = CFG_TUD_CONFIGURE_DWC2_DEFAULT;
    cfg.vbus_sensing = false;
    (void)tud_configure(rhport, TUD_CFGID_DWC2, &cfg);

#if defined(OTG_HS_IRQn)
    HAL_NVIC_SetPriority(OTG_HS_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(OTG_HS_IRQn);
#endif

    return true;
}

bool tinyusb_platform_init_host(uint8_t rhport)
{
    (void)rhport;
    return false;
}

void tinyusb_platform_after_init(void)
{
}

size_t tinyusb_platform_get_serial(uint16_t desc_str[], size_t max_chars)
{
    size_t index = 0U;
    write_hex_word(desc_str, &index, max_chars, HAL_GetUIDw0());
    write_hex_word(desc_str, &index, max_chars, HAL_GetUIDw1());
    write_hex_word(desc_str, &index, max_chars, HAL_GetUIDw2());
    return index;
}

uint32_t tusb_time_millis_api(void)
{
    return HAL_GetTick();
}

void OTG_HS_IRQHandler(void)
{
    tusb_int_handler(STM32_TINYUSB_RHPORT, true);
}

#if defined(OTG_FS_IRQn)
void OTG_FS_IRQHandler(void)
{
    tusb_int_handler(STM32_TINYUSB_RHPORT, true);
}
#endif

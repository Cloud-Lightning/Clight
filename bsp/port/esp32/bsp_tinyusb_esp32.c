#include "TinyUsbPlatform.h"

#include "tusb.h"

#include "esp_err.h"
#include "esp_idf_version.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 3, 0)
#include "esp_private/usb_phy.h"
#endif

static bool s_device_phy_ready = false;
static bool s_host_phy_ready = false;

static bool init_usb_phy(bool host)
{
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 3, 0)
    static usb_phy_handle_t device_phy = NULL;
    static usb_phy_handle_t host_phy = NULL;
    usb_phy_handle_t *target_phy = host ? &host_phy : &device_phy;

    if (*target_phy != NULL) {
        return true;
    }

    usb_phy_config_t phy_conf = {
        .controller = USB_PHY_CTRL_OTG,
#if defined(CONFIG_SOC_USB_UTMI_PHY_NUM) && CONFIG_SOC_USB_UTMI_PHY_NUM > 0
        .target = USB_PHY_TARGET_UTMI,
#else
        .target = USB_PHY_TARGET_INT,
#endif
        .otg_mode = host ? USB_OTG_MODE_HOST : USB_OTG_MODE_DEVICE,
        .otg_speed = USB_PHY_SPEED_UNDEFINED,
    };

    return usb_new_phy(&phy_conf, target_phy) == ESP_OK;
#else
    (void)host;
    return false;
#endif
}

bool tinyusb_platform_init(uint8_t rhport)
{
    if (rhport != 0U) {
        return false;
    }
    if (!s_device_phy_ready) {
        s_device_phy_ready = init_usb_phy(false);
    }
    return s_device_phy_ready;
}

bool tinyusb_platform_init_host(uint8_t rhport)
{
    if (rhport != 0U) {
        return false;
    }
    if (!s_host_phy_ready) {
        s_host_phy_ready = init_usb_phy(true);
    }
    return s_host_phy_ready;
}

void tinyusb_platform_after_init(void)
{
}

size_t tinyusb_platform_get_serial(uint16_t desc_str[], size_t max_chars)
{
    uint8_t mac[6] = {0};
    if ((desc_str == NULL) || (max_chars == 0U) || (esp_efuse_mac_get_default(mac) != ESP_OK)) {
        return 0U;
    }

    static const char hex[] = "0123456789ABCDEF";
    size_t index = 0U;
    for (size_t i = 0U; (i < sizeof(mac)) && ((index + 1U) < max_chars); ++i) {
        desc_str[index++] = hex[(mac[i] >> 4U) & 0x0FU];
        desc_str[index++] = hex[mac[i] & 0x0FU];
    }
    return index;
}

uint32_t tusb_time_millis_api(void)
{
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

void USB_OTG_ISR(void)
{
    tusb_int_handler(0U, true);
}

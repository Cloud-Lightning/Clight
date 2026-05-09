#include "bsp_usb_serial_jtag.h"

extern "C" {
#include "driver/usb_serial_jtag.h"
}

static bool s_usb_serial_jtag_initialized = false;

int32_t bsp_usb_serial_jtag_init(bsp_usb_serial_jtag_instance_t instance)
{
    if (instance >= BSP_USB_SERIAL_JTAG_MAX) {
        return BSP_ERROR_PARAM;
    }

    if (s_usb_serial_jtag_initialized) {
        return BSP_OK;
    }

    usb_serial_jtag_driver_config_t config = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    if (usb_serial_jtag_driver_install(&config) != ESP_OK) {
        return BSP_ERROR;
    }

    s_usb_serial_jtag_initialized = true;
    return BSP_OK;
}

int32_t bsp_usb_serial_jtag_write(bsp_usb_serial_jtag_instance_t instance, const uint8_t *data, uint32_t size, uint32_t timeout_ms)
{
    if ((instance >= BSP_USB_SERIAL_JTAG_MAX) || (data == nullptr) || (size == 0U)) {
        return BSP_ERROR_PARAM;
    }

    const auto init_status = bsp_usb_serial_jtag_init(instance);
    if (init_status != BSP_OK) {
        return init_status;
    }

    const auto written = usb_serial_jtag_write_bytes(data, size, pdMS_TO_TICKS(timeout_ms));
    if (written < 0) {
        return BSP_ERROR;
    }
    return (written == static_cast<int>(size)) ? BSP_OK : BSP_ERROR_TIMEOUT;
}

int32_t bsp_usb_serial_jtag_read(bsp_usb_serial_jtag_instance_t instance, uint8_t *data, uint32_t size, uint32_t timeout_ms)
{
    if ((instance >= BSP_USB_SERIAL_JTAG_MAX) || (data == nullptr) || (size == 0U)) {
        return BSP_ERROR_PARAM;
    }

    const auto init_status = bsp_usb_serial_jtag_init(instance);
    if (init_status != BSP_OK) {
        return init_status;
    }

    const auto read = usb_serial_jtag_read_bytes(data, size, pdMS_TO_TICKS(timeout_ms));
    if (read < 0) {
        return BSP_ERROR;
    }
    return (read > 0) ? BSP_OK : BSP_ERROR_TIMEOUT;
}

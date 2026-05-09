#ifndef BSP_USB_SERIAL_JTAG_H
#define BSP_USB_SERIAL_JTAG_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_USB_SERIAL_JTAG_LIST
#define BSP_USB_SERIAL_JTAG_LIST(X) \
    X(BSP_USB_SERIAL_JTAG_MAIN)
#endif

typedef enum {
#define BSP_USB_SERIAL_JTAG_ENUM_ITEM(name) name,
    BSP_USB_SERIAL_JTAG_LIST(BSP_USB_SERIAL_JTAG_ENUM_ITEM)
#undef BSP_USB_SERIAL_JTAG_ENUM_ITEM
    BSP_USB_SERIAL_JTAG_MAX
} bsp_usb_serial_jtag_instance_t;

int32_t bsp_usb_serial_jtag_init(bsp_usb_serial_jtag_instance_t instance);
int32_t bsp_usb_serial_jtag_write(bsp_usb_serial_jtag_instance_t instance, const uint8_t *data, uint32_t size, uint32_t timeout_ms);
int32_t bsp_usb_serial_jtag_read(bsp_usb_serial_jtag_instance_t instance, uint8_t *data, uint32_t size, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif

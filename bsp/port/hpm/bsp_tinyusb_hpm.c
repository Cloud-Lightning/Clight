#include "TinyUsbPlatform.h"

#include "board.h"
#include "bsp/board_api.h"
#include "tusb.h"

bool tinyusb_platform_init(uint8_t rhport)
{
    if (rhport == 0U) {
        board_init_usb(HPM_USB0);
        return true;
    }

#if defined(HPM_USB1)
    if (rhport == 1U) {
        board_init_usb(HPM_USB1);
        return true;
    }
#endif

    return false;
}

bool tinyusb_platform_init_host(uint8_t rhport)
{
    (void)rhport;
    return false;
}

void tinyusb_platform_after_init(void)
{
    board_init_after_tusb();
}

size_t tinyusb_platform_get_serial(uint16_t desc_str[], size_t max_chars)
{
    return board_usb_get_serial(desc_str, max_chars);
}

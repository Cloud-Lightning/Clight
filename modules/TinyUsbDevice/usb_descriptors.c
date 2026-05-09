#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "TinyUsbPlatform.h"
#include "tusb.h"

#define PID_MAP(itf, n) ((CFG_TUD_##itf) ? (1U << (n)) : 0U)
#define USB_VID 0xCafe
#define USB_PID (0x4000U | PID_MAP(CDC, 0) | PID_MAP(MSC, 1) | PID_MAP(HID, 2) | PID_MAP(MIDI, 3) | PID_MAP(VENDOR, 4))
#define USB_BCD 0x0200

static tusb_desc_device_t const desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = USB_BCD,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = USB_VID,
    .idProduct = USB_PID,
    .bcdDevice = 0x0100,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,
    .bNumConfigurations = 0x01,
};

uint8_t const *tud_descriptor_device_cb(void)
{
    return (uint8_t const *)&desc_device;
}

enum {
    ITF_NUM_CDC = 0,
    ITF_NUM_CDC_DATA,
    ITF_NUM_MSC,
    ITF_NUM_HID,
    ITF_NUM_VENDOR,
    ITF_NUM_TOTAL,
};

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_MSC_DESC_LEN + TUD_HID_DESC_LEN + TUD_VENDOR_DESC_LEN)

#if defined(TUD_ENDPOINT_ONE_DIRECTION_ONLY)
#define EPNUM_CDC_NOTIF 0x81
#define EPNUM_CDC_OUT 0x02
#define EPNUM_CDC_IN 0x83
#define EPNUM_MSC_OUT 0x04
#define EPNUM_MSC_IN 0x85
#define EPNUM_HID_IN 0x86
#define EPNUM_VENDOR_OUT 0x07
#define EPNUM_VENDOR_IN 0x88
#else
#define EPNUM_CDC_NOTIF 0x81
#define EPNUM_CDC_OUT 0x02
#define EPNUM_CDC_IN 0x82
#define EPNUM_MSC_OUT 0x03
#define EPNUM_MSC_IN 0x83
#define EPNUM_HID_IN 0x84
#define EPNUM_VENDOR_OUT 0x05
#define EPNUM_VENDOR_IN 0x85
#endif

static uint8_t const desc_hid_report[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(1)),
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(2)),
};

#define HID_REPORT_DESC_LEN (sizeof(desc_hid_report))

static uint8_t const desc_fs_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 5, EPNUM_MSC_OUT, EPNUM_MSC_IN, 64),
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 6, HID_ITF_PROTOCOL_NONE, HID_REPORT_DESC_LEN, EPNUM_HID_IN, CFG_TUD_HID_EP_BUFSIZE, 10),
    TUD_VENDOR_DESCRIPTOR(ITF_NUM_VENDOR, 7, EPNUM_VENDOR_OUT, EPNUM_VENDOR_IN, 64),
};

#if TUD_OPT_HIGH_SPEED
static uint8_t const desc_hs_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 512),
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 5, EPNUM_MSC_OUT, EPNUM_MSC_IN, 512),
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 6, HID_ITF_PROTOCOL_NONE, HID_REPORT_DESC_LEN, EPNUM_HID_IN, CFG_TUD_HID_EP_BUFSIZE, 10),
    TUD_VENDOR_DESCRIPTOR(ITF_NUM_VENDOR, 7, EPNUM_VENDOR_OUT, EPNUM_VENDOR_IN, 512),
};

static tusb_desc_device_qualifier_t const desc_device_qualifier = {
    .bLength = sizeof(tusb_desc_device_qualifier_t),
    .bDescriptorType = TUSB_DESC_DEVICE_QUALIFIER,
    .bcdUSB = USB_BCD,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .bNumConfigurations = 0x01,
    .bReserved = 0x00,
};

uint8_t const *tud_descriptor_device_qualifier_cb(void)
{
    return (uint8_t const *)&desc_device_qualifier;
}

uint8_t const *tud_descriptor_other_speed_configuration_cb(uint8_t index)
{
    (void)index;
    return (tud_speed_get() == TUSB_SPEED_HIGH) ? desc_fs_configuration : desc_hs_configuration;
}
#endif

uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;
#if TUD_OPT_HIGH_SPEED
    return (tud_speed_get() == TUSB_SPEED_HIGH) ? desc_hs_configuration : desc_fs_configuration;
#else
    return desc_fs_configuration;
#endif
}

enum {
    STRID_LANGID = 0,
    STRID_MANUFACTURER,
    STRID_PRODUCT,
    STRID_SERIAL,
    STRID_CDC,
    STRID_MSC,
    STRID_HID,
    STRID_VENDOR,
};

static char const *string_desc_arr[] = {
    (const char[]){0x09, 0x04},
    "Clight",
    "Clight TinyUSB CDC",
    NULL,
    "TinyUSB CDC",
    "TinyUSB MSC",
    "TinyUSB HID",
    "TinyUSB Vendor",
};

static uint16_t desc_str[33];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void)langid;
    size_t chr_count;

    if (index == STRID_LANGID) {
        memcpy(&desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else if (index == STRID_SERIAL) {
        chr_count = tinyusb_platform_get_serial(desc_str + 1, 32);
        if (chr_count == 0U) {
            static char const fallback_serial[] = "00000000";
            chr_count = strlen(fallback_serial);
            for (size_t i = 0; i < chr_count; ++i) {
                desc_str[1 + i] = fallback_serial[i];
            }
        }
    } else {
        if (index >= (sizeof(string_desc_arr) / sizeof(string_desc_arr[0]))) {
            return NULL;
        }

        const char *str = string_desc_arr[index];
        chr_count = strlen(str);
        if (chr_count > 32U) {
            chr_count = 32U;
        }

        for (size_t i = 0; i < chr_count; ++i) {
            desc_str[1 + i] = str[i];
        }
    }

    desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8U) | (2U * chr_count + 2U));
    return desc_str;
}

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void)instance;
    return desc_hid_report;
}

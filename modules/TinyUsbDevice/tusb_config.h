#ifndef TINYUSB_DEVICE_TUSB_CONFIG_H_
#define TINYUSB_DEVICE_TUSB_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BOARD_TUD_RHPORT
#define BOARD_TUD_RHPORT 0
#endif

#ifndef BOARD_TUD_MAX_SPEED
#define BOARD_TUD_MAX_SPEED OPT_MODE_HIGH_SPEED
#endif

#ifndef BOARD_TUH_RHPORT
#define BOARD_TUH_RHPORT 0
#endif

#ifndef BOARD_TUH_MAX_SPEED
#define BOARD_TUH_MAX_SPEED OPT_MODE_FULL_SPEED
#endif

#ifndef CFG_TUSB_MCU
#error CFG_TUSB_MCU must be defined by the platform build
#endif

#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS OPT_OS_NONE
#endif

#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG 0
#endif

#ifndef CFG_TUD_ENABLED
#define CFG_TUD_ENABLED 1
#endif

#ifndef CFG_TUH_ENABLED
#define CFG_TUH_ENABLED 0
#endif

#if BOARD_TUD_RHPORT == 0
#define CFG_TUSB_RHPORT0_MODE ((CFG_TUD_ENABLED ? (OPT_MODE_DEVICE | BOARD_TUD_MAX_SPEED) : 0) | \
                               ((CFG_TUH_ENABLED && (BOARD_TUH_RHPORT == 0)) ? (OPT_MODE_HOST | BOARD_TUH_MAX_SPEED) : 0))
#elif BOARD_TUD_RHPORT == 1
#define CFG_TUSB_RHPORT1_MODE ((CFG_TUD_ENABLED ? (OPT_MODE_DEVICE | BOARD_TUD_MAX_SPEED) : 0) | \
                               ((CFG_TUH_ENABLED && (BOARD_TUH_RHPORT == 1)) ? (OPT_MODE_HOST | BOARD_TUH_MAX_SPEED) : 0))
#else
#error BOARD_TUD_RHPORT must be 0 or 1
#endif
#define CFG_TUD_MAX_SPEED BOARD_TUD_MAX_SPEED

#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION __attribute__((section(".noncacheable.non_init")))
#endif

#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN __attribute__((aligned(4)))
#endif

#ifndef CFG_TUD_ENDPOINT0_SIZE
#define CFG_TUD_ENDPOINT0_SIZE 64
#endif

#ifndef CFG_TUD_TASK_QUEUE_SZ
#define CFG_TUD_TASK_QUEUE_SZ 16
#endif

#ifndef CFG_TUD_CDC
#define CFG_TUD_CDC 1
#endif
#ifndef CFG_TUD_MSC
#define CFG_TUD_MSC 1
#endif
#ifndef CFG_TUD_HID
#define CFG_TUD_HID 1
#endif
#define CFG_TUD_MIDI 0
#ifndef CFG_TUD_VENDOR
#define CFG_TUD_VENDOR 1
#endif

#define CFG_TUD_CDC_RX_BUFSIZE (TUD_OPT_HIGH_SPEED ? 512 : 64)
#define CFG_TUD_CDC_TX_BUFSIZE (TUD_OPT_HIGH_SPEED ? 512 : 64)
#define CFG_TUD_CDC_EP_BUFSIZE (TUD_OPT_HIGH_SPEED ? 512 : 64)

#ifndef CFG_TUD_MSC_EP_BUFSIZE
#define CFG_TUD_MSC_EP_BUFSIZE 512
#endif

#ifndef CFG_TUD_HID_EP_BUFSIZE
#define CFG_TUD_HID_EP_BUFSIZE 64
#endif

#ifndef CFG_TUD_VENDOR_RX_BUFSIZE
#define CFG_TUD_VENDOR_RX_BUFSIZE 512
#endif

#ifndef CFG_TUD_VENDOR_TX_BUFSIZE
#define CFG_TUD_VENDOR_TX_BUFSIZE 512
#endif

#ifndef CFG_TUH_HID
#define CFG_TUH_HID 1
#endif

#ifndef CFG_TUH_MSC
#define CFG_TUH_MSC 1
#endif

#ifndef CFG_TUH_DEVICE_MAX
#define CFG_TUH_DEVICE_MAX 4
#endif

#ifndef CFG_TUH_ENUMERATION_BUFSIZE
#define CFG_TUH_ENUMERATION_BUFSIZE 256
#endif

#ifdef __cplusplus
}
#endif

#endif

# TinyUsbDevice Module

Framework-facing TinyUSB device wrappers shared by HPM and STM32 targets.

Layout:

- `TinyUsbCdc.hpp/.cpp`
  C++ CDC device API used by applications.
- `TinyUsbMscDevice.hpp/.cpp`
  MSC device API with a block-device abstraction and a RAM-disk helper.
- `TinyUsbHidDevice.hpp/.cpp`
  HID keyboard/mouse report helpers.
- `TinyUsbVendorDevice.hpp/.cpp`
  Vendor bulk in/out helpers.
- `TinyUsbDeviceCore.hpp/.cpp`
  Shared one-time TinyUSB device initialization.
- `tusb_config.h`
  TinyUSB bare-metal CDC device configuration. The platform build must define
  `CFG_TUSB_MCU`, `BOARD_TUD_RHPORT`, and `BOARD_TUD_MAX_SPEED`.
- `usb_descriptors.c`
  Project USB descriptors.
- `TinyUsbPlatform.h`
  Platform hooks implemented under `Clight/bsp/port/<platform>`.

HPM builds can use the HPM SDK TinyUSB middleware enabled by `CONFIG_TINYUSB`.
STM32 builds use the repository copy in `Clight/vendor/tinyusb`. Keep official
TinyUSB source under `Clight/vendor/`; keep application-facing APIs here.
ESP32-S3 builds keep USB Serial/JTAG as a separate debug channel and can opt in
to TinyUSB device support with `ESP32_ENABLE_TINYUSB_DEVICE`.

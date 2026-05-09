# Clight

Clight is a lightweight embedded C/C++ framework for keeping application code independent from chip vendor SDKs.

The framework provides one public API surface for common MCU peripherals, while STM32, HPM, and ESP32 differences stay in BSP ports and board capability tables.

## What This Repository Contains

- `bsp/interface`: stable C BSP interfaces and status/error types.
- `bsp/port/stm32`: STM32 HAL/LL backed BSP implementations.
- `bsp/port/hpm`: HPM SDK backed BSP implementations.
- `bsp/port/esp32`: ESP-IDF backed BSP implementations.
- `bsp/chip`: board/chip capability declarations.
- `bsp/api`: generated C++ API wrappers used by application code.
- `modules`: reusable device and service modules such as BMI088, WS2812, TinyUSB, EtherCAT/LwIP wrappers, displays, and self-test.
- `platforms`: minimal platform entry projects and board glue.
- `tools/clight_codegen`: API generation tool.
- `docs`: architecture notes, callback/DMA notes, and support matrices.

## What Is Intentionally Not Included

This public repository intentionally does not include vendor SDK source trees or generated build outputs:

- STM32CubeMX generated `Core/Drivers` folders are not published.
- HPM SDK source trees are not published.
- ESP-IDF build folders and generated `sdkconfig` files are not published.
- `vendor/`, `build/`, `out/`, binary images, map files, object files, and local tool caches are ignored.
- Secrets, tokens, passwords, Wi-Fi credentials, and private machine paths must stay outside the repository.

## Supported Direction

Clight separates the project into clear layers:

```text
Application / Modules
        |
Generated C++ API in bsp/api
        |
Unified C BSP interface in bsp/interface
        |
Platform BSP port in bsp/port/<platform>
        |
Vendor SDK or HAL layer
```

Application code should include headers such as:

```cpp
#include "Gpio.hpp"
#include "Spi.hpp"
#include "Uart.hpp"
#include "Can.hpp"
#include "Adc.hpp"
```

Application code should not directly include STM32 HAL, HPM SDK, or ESP-IDF headers unless it is inside a platform BSP port.

## Current Platform Coverage

- STM32H7: GPIO, Timer/PWM, SPI, I2C, UART, ADC16, FDCAN, CRC, RNG, RTC, watchdog interfaces, and unsupported-return stubs for unconfigured heavy peripherals.
- HPM5E31: GPIO, Timer/PWM/PWMV2, SPI, I2C, UART, ADC16, MCAN, ENET/ECAT interfaces, CRC/EWDG, and board capability declarations.
- ESP32-S3: GPIO, UART, SPI, I2C, ADC, Timer/PWM/LEDC, RMT/PCNT placeholders, USB Serial JTAG, Wi-Fi service, HTTP service, BLE service skeleton, and TinyUSB platform hooks.

Unsupported board resources must return `Status::Unsupported`; invalid parameters must return `Status::Param`. The framework should not silently fake success.

## Regenerating Public API

The C++ API wrappers are generated from YAML manifests:

```powershell
python tools/clight_codegen/generate_cpp_api.py --config config/clight/stm32.yaml --out-root .
python tools/clight_codegen/generate_cpp_api.py --config config/clight/hpm.yaml --out-root .
python tools/clight_codegen/generate_cpp_api.py --config config/clight/esp32.yaml --out-root .
```

## Build Notes

This repository is framework-first. To build a real board project, provide the vendor SDK externally:

- STM32: generate or provide CubeMX `Core/Drivers`, startup, linker, and HAL configuration in your board project.
- HPM: set up HPM SDK in the environment and point the platform CMake to it.
- ESP32: build under ESP-IDF with the IDF environment activated.

The checked-in `platforms` folder is a reference entry point, not a full vendor SDK mirror.

## Repository Hygiene

Before publishing or pushing changes:

```powershell
git status --short
git ls-files
```

Check that no files such as `sdkconfig`, `.env`, `*.pem`, `*.key`, `build/`, `vendor/`, `Core/`, or `Drivers/` are staged.

## Website

Project site:

https://github.com/Cloud-Lightning/Clight_Web

Framework repository:

https://github.com/Cloud-Lightning/Clight

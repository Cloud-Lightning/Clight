# Architecture

Clight keeps application code away from vendor SDK details.

```text
Application / reusable modules
        |
Clight C++ API: bsp/api
        |
Clight C BSP interface: bsp/interface
        |
Platform port: bsp/port/stm32, bsp/port/hpm, bsp/port/esp32
        |
Vendor HAL / SDK / ESP-IDF
```

## Layer Ownership

- `bsp/api`: public C++ wrappers such as `Gpio.hpp`, `Spi.hpp`, `Uart.hpp`, `Can.hpp`, and `Adc.hpp`. Application code should include these headers.
- `bsp/interface`: stable C ABI used by the C++ wrappers. This layer defines status codes, instance IDs, config structs, feature bits, callbacks, and common behavior.
- `bsp/port/<platform>`: platform implementation. STM32 HAL/LL, HPM SDK, and ESP-IDF headers are allowed here.
- `bsp/chip/<platform>`: board or chip capability tables. This layer says what the current board has and what must return `Unsupported`.
- `modules`: reusable devices and services built on top of the public API, for example BMI088, WS2812, TinyUSB wrappers, display helpers, and self-test.
- `platforms`: minimal entry templates. This folder is not a vendor-generated board package.

## Status Rules

All ports must follow the same behavior:

- Return `Ok` only when the operation really completed or was really started.
- Return `Unsupported` when the board, pinmux, peripheral, DMA channel, PHY, or protocol stack is not present.
- Return `Param` for invalid arguments such as null buffers, invalid channels, or impossible lengths.
- Do not silently fall back from DMA to interrupt when the caller explicitly requested DMA.
- Do not print from BSP code unless guarded by a debug switch.

## Platform-Specific Features

Generic APIs expose cross-platform behavior. Platform-only helpers must be optional extensions, not part of the core API.

Examples:

- HPM ECAT, TRGM, and special PWMV2 helpers belong in HPM-specific extension code.
- STM32 ADC injected trigger helpers belong in STM32-specific extension code.
- ESP32 Wi-Fi, BLE, HTTP server, RMT, and PCNT services can have modules, but they should not pollute the basic GPIO/SPI/UART API.

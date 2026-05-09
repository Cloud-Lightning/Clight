# Clight

Clight is a lightweight embedded C/C++ framework for keeping application and module code independent from chip vendor SDKs.

The public repository contains the framework layer and the vendor-origin source that Clight intentionally maintains as modified local copies. It does not publish generated HPM board packages, STM32CubeMX projects, ESP-IDF build output, full SDK mirrors, firmware images, or private credentials.

## Repository Contents

- `bsp/interface`: unified C BSP interfaces and status/error types.
- `bsp/api`: public C++ API wrappers used by application code.
- `bsp/port/stm32`: STM32 HAL/LL backed BSP implementations.
- `bsp/port/hpm`: HPM SDK backed BSP implementations.
- `bsp/port/esp32`: ESP-IDF backed BSP implementations.
- `bsp/chip`: chip and board capability declarations.
- `modules`: reusable devices and services built on top of Clight APIs.
- `vendor`: maintained third-party or vendor-origin source used by Clight, such as adapted HPM lwIP/EtherCAT glue and TinyUSB.
- `platforms`: minimal platform entry templates only.
- `tools/clight_codegen`: C++ API generation tool.
- `docs`: architecture, generated-file placement, porting, and release rules.

## Architecture

```text
Application / modules
        |
Clight C++ API: bsp/api
        |
Clight C BSP interface: bsp/interface
        |
Platform port: bsp/port/<platform>
        |
Vendor HAL / SDK / ESP-IDF
```

Application code should include public API headers such as:

```cpp
#include "Gpio.hpp"
#include "Spi.hpp"
#include "Uart.hpp"
#include "Can.hpp"
```

Application code should not directly include STM32 HAL, HPM SDK, or ESP-IDF headers unless it is inside a platform BSP port or a platform-specific extension.

## Platform Templates

The checked-in `platforms` folder is intentionally minimal:

- HPM board files must be generated locally into `platforms/hpm/boards/<board_name>/`.
- STM32CubeMX output must be generated locally into `platforms/stm32/board/<board_name>/` or kept in a product repository.
- ESP32 `sdkconfig`, `build/`, credentials, and managed components stay local.

See `docs/03_generated_files.md` before creating a real board project.

## Behavior Contract

All BSP ports follow the same return rules:

- `Ok`: the operation really succeeded or was really started.
- `Unsupported`: the resource, pinmux, DMA channel, PHY, protocol stack, or board feature is not available.
- `Param`: caller passed invalid arguments.

The framework must not fake success and must not hide platform differences in the API layer.

## Dependencies

Clight does not vendor full chip SDKs. A real product workspace supplies:

- STM32CubeMX/CubeCLT generated files for STM32.
- HPM SDK and generated board files for HPM.
- ESP-IDF environment for ESP32.
- Optional protocol stacks or vendor packages through the product workspace when they are not already maintained under `vendor/`.

## Documentation

Start with:

- `docs/01_architecture.md`
- `docs/02_platform_workspace.md`
- `docs/03_generated_files.md`
- `docs/04_porting_checklist.md`
- `docs/05_release_policy.md`

## Repository Hygiene

Before publishing:

```powershell
git status --short
git ls-files
git grep -n -i "password\|secret\|token\|ssid\|private key"
```

Do not stage generated SDK output such as `build/`, `out/`, `Core/`, `Drivers/`, `sdkconfig`, `.hpmpc`, firmware images, map files, certificates, private keys, passwords, or tokens. Only stage `vendor/` files when they are source files that Clight intentionally maintains.

## Links

Framework repository: <https://github.com/Cloud-Lightning/Clight>

Website repository: <https://github.com/Cloud-Lightning/Clight_Web>

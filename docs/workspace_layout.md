# Workspace Layout

This project has been cleaned up so the top-level source layout is simple:

- `app`
- `board`
- `framework`

That means when you open `E:\HPM\my_project\HPM5E31_CODE`, the real source code is now grouped into only these three visible folders.

## Top Level

### `app`

Main HPM application entry and app-side build logic.

Use this when you care about:

- the HPM application entry
- app-side feature selection
- HPM app build flow

### `board`

Board-specific support such as:

- board startup
- clocks
- pinmux
- board-local C files

### `framework`

All reusable framework-side code now lives here.

Inside `framework` you will find:

- `bsp`
  Unified BSP abstraction, platform ports, chip/board mappings
- `modules`
  Reusable device and component sources, one folder per module, such as `Clight/modules/Bmi088`, `Clight/modules/Mfrc522`, `Clight/modules/TopicBus`
- `config`
  `clight` YAML configuration files
- `docs`
  Framework and support-matrix documentation
- `manifest`
  Generated API manifests
- `platforms`
  Platform-specific standalone project entries such as `platforms/esp32` and `platforms/stm32`
- `tools`
  Code generation and maintenance scripts

## What was cleaned out

The following local/generated directories were intentionally removed from the top level:

- old build directories
- local preset/test folders
- screenshot folder
- local editor experiment folders

Only source-oriented content was intentionally left behind.

## Suggested mental model

If you only care about Clight/source code, focus on:

- `Clight/bsp`
- `Clight/modules`
- `Clight/config`
- `Clight/bsp/api`
- `Clight/manifest`
- `Clight/tools`
- `Clight/platforms/stm32`
- `Clight/platforms/esp32`

If you only care about low-level hardware configuration, focus on:

- `board`
- `Clight/bsp/chip/*/board_config.h`
- `Clight/platforms/stm32/vendor/*/Core/Src/*.c`
- `Clight/platforms/esp32/main/*`

If you only care about application code, focus on:

- `app`
- `Clight/platforms/stm32/main`
- `Clight/platforms/esp32/main`

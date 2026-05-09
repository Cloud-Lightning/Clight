# STM32 / ESP32 Full-Chip Support Matrix

This document tracks the current `clight` full-chip style profiles:

- `stm32h723vgtx_core_control`
- `esp32s3_devkitc`

Status legend:

- `implemented`: public API exists and current profile has a working implementation
- `unsupported on current board`: public API exists but current board/profile returns `BSP_ERROR_UNSUPPORTED`
- `driver-only skeleton`: public API, wrapper, and platform glue exist, but only the minimum init path is provided

## Official Sources

### STM32

- Official baseline: `STM32CubeH7 v1.13.0`
- Vendor path in repo: `stm32/vendor/stm32cubeh7_official`
- Official HAL driver source: `Drivers/STM32H7xx_HAL_Driver`
- Official device/CMSIS source: `Drivers/CMSIS/Device/ST/STM32H7xx`

### ESP32

- Official baseline: local installed `ESP-IDF v5.4.1`
- Build currently validated against the external IDF at `E:\Users\12715\esp\v5.4.1\esp-idf`
- No full IDF vendor snapshot is stored in repo

## STM32H723 CORE_CONTROL

### First family + internal expansions

| Module | Public header | Status | Notes |
| --- | --- | --- | --- |
| `gpio` | `Gpio.hpp` | `implemented` | Chip-style GPIO names such as `Pc13`, `Pc14`, `Pc0` |
| `timer` | `Timer.hpp` | `implemented` | `Tim2` |
| `pwm` | `Pwm.hpp` | `unsupported on current board` | `Tim1`, no current output mapping |
| `adc` | `Adc.hpp` | `unsupported on current board` | `Adc1`, no current routing |
| `uart` | `Uart.hpp` | `implemented` | `Lpuart1`, `Usart10` |
| `spi` | `Spi.hpp` | `implemented` | `Spi2` active, `Spi1` exported |
| `i2c` | `I2c.hpp` | `unsupported on current board` | `I2c1` |
| `can` | `Can.hpp` | `implemented` | `Fdcan1`, `Fdcan2` |
| `encoder` | `Encoder.hpp` | `unsupported on current board` | `Tim3` |
| `dma` | `Dma.hpp` | `implemented` | `Dma1Stream3` |
| `crc` | `Crc.hpp` | `implemented` | hardware CRC |
| `watchdog` | `Watchdog.hpp` | `implemented` | `Iwdg1`, deinit stays unsupported |
| `rng` | `Rng.hpp` | `implemented` | HAL RNG |
| `rtc` | `Rtc.hpp` | `driver-only skeleton` | exported, currently unsupported |
| `wwdg` | `Wwdg.hpp` | `driver-only skeleton` | exported, currently unsupported |
| `cordic` | `Cordic.hpp` | `implemented` | clock-enable/init path |
| `fmac` | `Fmac.hpp` | `implemented` | clock-enable/init path |
| `dma2d` | `Dma2d.hpp` | `implemented` | clock-enable/init path |
| `mdma` | `Mdma.hpp` | `implemented` | clock-enable/init path |
| `dac` | `Dac.hpp` | `unsupported on current board` | `Dac1`, no mapped output |

### Second family

| Module | Public header | Status | Notes |
| --- | --- | --- | --- |
| `comp` | `Comp.hpp` | `unsupported on current board` | `Comp1`, `Comp2` |
| `dcmi` | `Dcmi.hpp` | `unsupported on current board` | `Dcmi` |
| `dfsdm` | `Dfsdm.hpp` | `unsupported on current board` | `Dfsdm1` |
| `eth` | `Eth.hpp` | `unsupported on current board` | exposed as `Eth` using `bsp_enet` abstraction |
| `hcd` | `Hcd.hpp` | `unsupported on current board` | `HcdHs` |
| `ltdc` | `Ltdc.hpp` | `unsupported on current board` | `Ltdc` |
| `mmc` | `Mmc.hpp` | `unsupported on current board` | `Mmc1`, `Mmc2` |
| `opamp` | `Opamp.hpp` | `unsupported on current board` | `Opamp1`, `Opamp2` |
| `ospi` | `Ospi.hpp` | `unsupported on current board` | `Ospi1`, `Ospi2` |
| `pcd` | `Pcd.hpp` | `unsupported on current board` | `PcdHs` |
| `pssi` | `Pssi.hpp` | `unsupported on current board` | `Pssi` |
| `sai` | `Sai.hpp` | `unsupported on current board` | `Sai1`, `Sai4` |
| `sd` | `Sd.hpp` | `unsupported on current board` | `Sdmmc1`, `Sdmmc2` |
| `sdio` | `Sdio.hpp` | `unsupported on current board` | `Sdio1`, `Sdio2` |
| `smartcard` | `Smartcard.hpp` | `unsupported on current board` | all exported USART-backed instances |
| `smbus` | `Smbus.hpp` | `unsupported on current board` | all exported I2C-backed instances |
| `usart` | `Usart.hpp` | `driver-only skeleton` | `Usart10` is marked available in profile, others exported but unavailable |

### Not exported on STM32H723 because the chip does not expose them

- `jpeg`
- `qspi`

These remain supported by the generic generator/module set for other platforms, but are not part of this chip profile.

## ESP32-S3 DEVKITC

### Current exported families

| Module | Public header | Status | Notes |
| --- | --- | --- | --- |
| `gpio` | `Gpio.hpp` | `implemented` | `Gpio0`, `Gpio2` |
| `timer` | `Timer.hpp` | `implemented` | `Gptimer0` |
| `pwm` | `Pwm.hpp` | `implemented` | `Ledc0` |
| `adc` | `Adc.hpp` | `implemented` | `Adc1Ch0` |
| `uart` | `Uart.hpp` | `implemented` | `Uart0`, `Uart1` |
| `spi` | `Spi.hpp` | `implemented` | `Spi2` |
| `i2c` | `I2c.hpp` | `implemented` | `I2c0` |
| `tsens` | `Tsens.hpp` | `implemented` | IDF temperature sensor driver |
| `usb_serial_jtag` | `UsbSerialJtag.hpp` | `implemented` | IDF USB Serial/JTAG driver |
| `pcnt` | `Pcnt.hpp` | `unsupported on current board` | exported |
| `rmt` | `Rmt.hpp` | `unsupported on current board` | exported |
| `mcpwm` | `Mcpwm.hpp` | `unsupported on current board` | exported |
| `sdspi` | `Sdspi.hpp` | `unsupported on current board` | exported |
| `lcd` | `Lcd.hpp` | `unsupported on current board` | exported |
| `wifi` | `Wifi.hpp` | `unsupported on current board` | exported |
| `touch` | `Touch.hpp` | `unsupported on current board` | exported |
| `i2s` | `I2s.hpp` | `unsupported on current board` | exported |
| `dac` | `Dac.hpp` | `unsupported on current board` | exported |

### Second family

| Module | Public header | Status | Notes |
| --- | --- | --- | --- |
| `ana_cmpr` | `AnaCmpr.hpp` | `unsupported on current board` | reuses generic ACMP abstraction |
| `cam` | `Cam.hpp` | `unsupported on current board` | controller/sensor path not mapped on current board |
| `jpeg` | `Jpeg.hpp` | `implemented` | exported as `Jpeg0` skeleton/available init path |
| `parlio` | `Parlio.hpp` | `unsupported on current board` | exported |
| `ppa` | `Ppa.hpp` | `implemented` | exported as `Ppa0` skeleton/available init path |
| `sdio` | `Sdio.hpp` | `unsupported on current board` | exported |
| `sdmmc` | `Sdmmc.hpp` | `unsupported on current board` | exported |
| `sdm` | `Sdm.hpp` | `unsupported on current board` | exported as `Sdm0` |

## Validation Snapshot

- `STM32`:
  - code generation succeeds for `stm32h723vgtx_core_control`
  - `build_stm32` links successfully against `stm32/vendor/stm32cubeh7_official`
- `ESP32`:
  - code generation succeeds for `esp32s3_devkitc`
  - `build_esp32_ninja5` completes successfully against official external `ESP-IDF`

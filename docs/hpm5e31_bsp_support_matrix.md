# HPM5E31 BSP Support Matrix

## Sample To BSP Mapping

| SDK sample dir | BSP family |
| --- | --- |
| `adc` | `adc16` |
| `can` | `mcan` |
| `dma` | `dmav2` |
| `wdg` | `ewdg` |
| `pwm` | `pwm` (GPTMR simple PWM) |
| `pwmv2` | `pwmv2` |
| `qei` | `qeiv2` |
| `qeo` | `qeov2` |
| `pllctl` | `pllctlv2` |
| `lin` | `uart` mode capability |

## BSP Status

| Family | BSP status | Board status | Smoke status |
| --- | --- | --- | --- |
| `gpio` | implemented | mapped | default app retained |
| `uart` | implemented | `UART0` + `UART3` mapped | `comm` |
| `enet` | retained | mapped | default app retained |
| `ecat` | retained | mapped | separate build mode |
| `pwm` | retained | mapped | default app retained |
| `gptmr` | implemented | mapped | `timing` |
| `pwmv2` | implemented | mapped | `timing` |
| `adc16` | implemented | mapped | `analog` |
| `spi` | implemented | mapped | `comm` |
| `i2c` | implemented | mapped | `comm` |
| `mcan` | implemented | mapped | `comm` |
| `qeiv2` | implemented | mapped | `control` |
| `crc` | implemented | internal | `system` |
| `dmav2` | implemented | internal | `system` |
| `ewdg` | implemented | internal | `system` |
| `tsns` | implemented | internal | `system` |
| `mchtmr` | implemented | internal | `system` |
| `l1c` | implemented | internal | `system` |
| `lobs` | implemented | internal | `system` |
| `mbx` | implemented | internal | `system` |
| `plic` | implemented | internal | `system` |
| `plb` | implemented | internal | `system` |
| `ptpc` | implemented | internal | `system` |
| `synt` | implemented | internal | `system` |
| `pllctlv2` | implemented | internal | `system` |
| `ppor` | implemented | internal | `system` |
| `pdgo` | implemented | internal | `system` |
| `acmp` | implemented | no board pinmux | `analog`, returns `BSP_ERROR_UNSUPPORTED` |
| `qeov2` | implemented | no board pinmux | `control`, returns `BSP_ERROR_UNSUPPORTED` |
| `sdm` | implemented | no board pinmux | `control`, returns `BSP_ERROR_UNSUPPORTED` |
| `eui` | implemented | no board pinmux | `system`, returns `BSP_ERROR_UNSUPPORTED` |
| `owr` | implemented | no board pinmux | `system`, returns `BSP_ERROR_UNSUPPORTED` |
| `ppi` | implemented | no board pinmux | `system`, returns `BSP_ERROR_UNSUPPORTED` |

## Board Mapping Notes

| Family | Mapping |
| --- | --- |
| `ADC16` | `ADC0` default `CH1` |
| `GPTMR capture` | `PA22` |
| `GPTMR compare` | `PC16` |
| `PWMV2` | `PWM0` on `PD00..PD05` |
| `QEIV2` | `QEI1` on `PA27/PB31/PB30` |
| `UART3` | `PC14/PC15` |
| `SPI1` | `PC10/PC11/PC12/PC13` |
| `I2C3` | `PD06/PD07` |
| `MCAN2` | `PC08/PC09` |

## Build Commands

```powershell
cmake -S E:\HPM\my_project\HPM5E31_CODE\app -B E:\HPM\my_project\HPM5E31_CODE\build
cmake --build E:\HPM\my_project\HPM5E31_CODE\build --target all -j 4

cmake -G Ninja -S E:\HPM\my_project\HPM5E31_CODE\app -B E:\HPM\my_project\HPM5E31_CODE\build_smoke_ninja `
  -DBOARD_SEARCH_PATH=E:\HPM\my_project\HPM5E31_CODE\Clight\platforms\hpm\boards `
  -DAPP_ENABLE_BSP_SMOKE=ON `
  -DAPP_BSP_SMOKE_GROUP=all
cmake --build E:\HPM\my_project\HPM5E31_CODE\build_smoke_ninja --target all -j 4
```

## Hardware Notes

- `SPI1` smoke assumes local loopback or equivalent wiring.
- `I2C3` smoke only guarantees init and transaction path. A successful read still needs a real slave at the configured address.
- `ACMP / QEOV2 / SDM / EUI / OWR / PPI` are intentionally exported as BSP families, but this board revision has no ready pinmux route for them, so `init()` returns `BSP_ERROR_UNSUPPORTED`.
- `MCAN2` smoke uses internal loopback and does not require an external transceiver pair.

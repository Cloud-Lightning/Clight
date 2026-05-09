# config/clight

The files in this directory are not low-level BSP drivers.
They are public API generation configs for `clight`.

Each config does two things:

- selects which modules are exported to the public C++ API
- maps public instance names to internal BSP identifiers

These files use JSON syntax saved as `.yaml`.
That is intentional. The generator uses `json.loads(...)`.

## Minimal fields

- `board`
  output profile name, used by the manifest file name
- `modules`
  the public modules to export
- `modules.<name>.instances`
  map from public name to internal BSP enum

Example:

```json
{
  "board": "stm32h723vgtx",
  "modules": {
    "timer": {
      "instances": {
        "Tim2": "BSP_TIM_MAIN"
      }
    },
    "uart": {
      "instances": {
        "Lpuart1": "BSP_UART_DEBUG",
        "Usart10": "BSP_UART_APP"
      }
    }
  }
}
```

## generated override block

Use `generated` when the public class name stays the same but the internal
platform BSP interface type or helper logic changes.

Examples:

- HPM: `Timer -> bsp_gptmr_*`
- HPM: `Pwm -> bsp_pwm_*` on boards that only expose simple GPTMR PWM
- STM32: `Timer -> bsp_tim_*`
- HPM: `Can -> bsp_mcan_*`
- STM32: `Can -> bsp_fdcan_*`

Typical override fields:

- `include`
- `internal_id_type`
- `methods`
- `extra_helpers`

## Generate commands

From repository root:

```powershell
python Clight/tools/clight_codegen/generate_cpp_api.py --config Clight/config/clight/hpm.yaml --out-root Clight
python Clight/tools/clight_codegen/generate_cpp_api.py --config Clight/config/clight/stm32.yaml --out-root Clight
python Clight/tools/clight_codegen/generate_cpp_api.py --config Clight/config/clight/esp32.yaml --out-root Clight
```

Generated outputs:

- `Clight/bsp/api`
- `Clight/manifest/<board>.json`

## Add a new profile

1. Add `Clight/config/clight/<profile>.yaml`
2. Set the `board` field
3. Add `instances` for each exported module
4. Add `generated` overrides only when the default template is not enough
5. Run the generator
6. Add `Clight/bsp/api` to the target include path

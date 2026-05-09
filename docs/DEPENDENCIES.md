# External Dependencies

Clight does not vendor full chip SDKs in this repository.

## STM32

Use STM32CubeMX or an existing board project to provide:

- `Core/Inc`
- `Core/Src`
- `Drivers/CMSIS`
- `Drivers/STM32H7xx_HAL_Driver`
- startup file
- linker script

Keep generated CubeMX output in the consuming board project, not in this framework repository.

## HPM

Install HPM SDK separately and expose it through your normal HPM SDK environment variables or CMake toolchain setup.

Optional HPM protocol stacks such as lwIP or EtherCAT SSC should be kept as external dependencies or mirrored in a private/vendor repository if licensing requires it.

## ESP32

Install ESP-IDF separately. Keep generated folders such as `build/`, `build_*`, and `sdkconfig` out of source control. Commit only stable defaults such as `sdkconfig.defaults`.

## Secrets

Do not commit Wi-Fi passwords, access tokens, certificates, private keys, or board-local credentials. Use local environment variables, private config files ignored by Git, or deployment-time provisioning.

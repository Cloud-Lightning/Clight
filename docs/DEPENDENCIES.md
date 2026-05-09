# Dependencies

Clight does not vendor the full chip SDKs in this public repository.

Required external dependencies depend on the target:

- STM32: STM32CubeMX or STM32CubeCLT generated project files and STM32 HAL/LL drivers.
- HPM: HPM SDK and a locally generated HPM board package.
- ESP32: ESP-IDF environment.
- EtherCAT/lwIP/TinyUSB/vendor protocol stacks: supplied by the product workspace or a separately managed dependency package when needed.

The framework API and BSP interfaces are kept in this repository. Large SDK source trees, generated files, and local tool outputs are intentionally excluded.

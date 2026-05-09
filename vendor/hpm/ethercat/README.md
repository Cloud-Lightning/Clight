# EtherCAT Local Vendor Layout

This directory contains the project-local EtherCAT slave stack and HPM porting layer.

Current dependency boundary:

- Required: `HPM_SDK_BASE` (drivers, board support, EEPROM emulation, toolchain integration)
- Not required: `hpm_sdk/samples/ethercat/ecat_io`

Local layout:

- `Clight/vendor/ethercat/application/`
  Local application object dictionary and PDO handling (`digital_io.*`)
- `Clight/vendor/ethercat/port/`
  HPM ESC hardware port and EEPROM emulation glue
- `Clight/vendor/ethercat/SSC/Src/`
  Vendored SSC generated source files used by this project
- `Clight/vendor/ethercat/ecat_config.h`
  Board-level EtherCAT configuration

When regenerating SSC code, replace the generated files in `Clight/vendor/ethercat/SSC/Src/` and keep the local HPM port/application files intact.

Reference material may still come from the official SDK sample docs, but the build should not depend on the sample directory being present.

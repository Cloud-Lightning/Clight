# EEPROM Emulation Local Vendor

This directory vendors the HPM SDK EEPROM emulation component into the project framework.

Included files:

- `eeprom_emulation.c`
- `eeprom_emulation.h`
- `hpm_nor_flash.c`
- `hpm_nor_flash.h`

Notes:

- `user_config.h` is provided by the consumer vendor tree. For EtherCAT, it is located in `Clight/vendor/ethercat/user_config.h`.
- HPM SDK drivers and ROM/XPI support are still required.

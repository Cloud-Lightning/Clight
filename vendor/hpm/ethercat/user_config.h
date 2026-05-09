/*
 * Copyright (c) 2026
 *
 * Project-local EEPROM emulation config for EtherCAT SSC.
 * Based on the HPM SDK EtherCAT ecat_io sample configuration,
 * but vendored here so the build does not depend on the sample tree.
 */

#ifndef _USER_CONFIG_H
#define _USER_CONFIG_H

#include "ecat_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define E2P_DEBUG_LEVEL    E2P_DEBUG_LEVEL_WARN
#define EEPROM_MAX_VAR_CNT (ESC_EEPROM_SIZE)

#ifdef __cplusplus
}
#endif

#endif

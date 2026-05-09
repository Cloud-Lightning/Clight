/*
 * EtherCAT PHY wrapper for project-local board configuration.
 */

#ifndef APP_HPM_ECAT_PHY_H
#define APP_HPM_ECAT_PHY_H

#include "ecat_config.h"
#include "hpm_esc_drv.h"

#ifdef __cplusplus
extern "C" {
#endif

void ecat_phy_reset(void);
void ecat_phy_keep_reset(void);
hpm_stat_t ecat_phy_config(ESC_Type *ptr);

#ifdef __cplusplus
}
#endif

#endif /* APP_HPM_ECAT_PHY_H */


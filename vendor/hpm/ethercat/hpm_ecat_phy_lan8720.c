/*
 * EtherCAT PHY wrapper for LAN8720A on custom HPM5E31 board.
 */

#include "hpm_ecat_phy.h"
#include "board.h"
#include "hpm_gpio_drv.h"

#define LAN8720_RESET_HOLD_TIME_MS (1U)
#define LAN8720_RESET_WAIT_TIME_MS (10U)

void ecat_phy_reset(void)
{
    gpio_write_pin(BOARD_ECAT_PHY0_RESET_GPIO,
                   BOARD_ECAT_PHY0_RESET_GPIO_PORT_INDEX,
                   BOARD_ECAT_PHY0_RESET_PIN_INDEX,
                   BOARD_ECAT_PHY_RESET_LEVEL);
#if defined(BOARD_ECAT_SUPPORT_PORT1) && BOARD_ECAT_SUPPORT_PORT1
    gpio_write_pin(BOARD_ECAT_PHY1_RESET_GPIO,
                   BOARD_ECAT_PHY1_RESET_GPIO_PORT_INDEX,
                   BOARD_ECAT_PHY1_RESET_PIN_INDEX,
                   BOARD_ECAT_PHY_RESET_LEVEL);
#endif
#if defined(BOARD_ECAT_SUPPORT_PORT2) && BOARD_ECAT_SUPPORT_PORT2
    gpio_write_pin(BOARD_ECAT_PHY2_RESET_GPIO,
                   BOARD_ECAT_PHY2_RESET_GPIO_PORT_INDEX,
                   BOARD_ECAT_PHY2_RESET_PIN_INDEX,
                   BOARD_ECAT_PHY_RESET_LEVEL);
#endif

    board_delay_ms(LAN8720_RESET_HOLD_TIME_MS);

    gpio_write_pin(BOARD_ECAT_PHY0_RESET_GPIO,
                   BOARD_ECAT_PHY0_RESET_GPIO_PORT_INDEX,
                   BOARD_ECAT_PHY0_RESET_PIN_INDEX,
                   !BOARD_ECAT_PHY_RESET_LEVEL);
#if defined(BOARD_ECAT_SUPPORT_PORT1) && BOARD_ECAT_SUPPORT_PORT1
    gpio_write_pin(BOARD_ECAT_PHY1_RESET_GPIO,
                   BOARD_ECAT_PHY1_RESET_GPIO_PORT_INDEX,
                   BOARD_ECAT_PHY1_RESET_PIN_INDEX,
                   !BOARD_ECAT_PHY_RESET_LEVEL);
#endif
#if defined(BOARD_ECAT_SUPPORT_PORT2) && BOARD_ECAT_SUPPORT_PORT2
    gpio_write_pin(BOARD_ECAT_PHY2_RESET_GPIO,
                   BOARD_ECAT_PHY2_RESET_GPIO_PORT_INDEX,
                   BOARD_ECAT_PHY2_RESET_PIN_INDEX,
                   !BOARD_ECAT_PHY_RESET_LEVEL);
#endif

    board_delay_ms(LAN8720_RESET_WAIT_TIME_MS);
}

void ecat_phy_keep_reset(void)
{
    gpio_write_pin(BOARD_ECAT_PHY0_RESET_GPIO,
                   BOARD_ECAT_PHY0_RESET_GPIO_PORT_INDEX,
                   BOARD_ECAT_PHY0_RESET_PIN_INDEX,
                   BOARD_ECAT_PHY_RESET_LEVEL);
#if defined(BOARD_ECAT_SUPPORT_PORT1) && BOARD_ECAT_SUPPORT_PORT1
    gpio_write_pin(BOARD_ECAT_PHY1_RESET_GPIO,
                   BOARD_ECAT_PHY1_RESET_GPIO_PORT_INDEX,
                   BOARD_ECAT_PHY1_RESET_PIN_INDEX,
                   BOARD_ECAT_PHY_RESET_LEVEL);
#endif
#if defined(BOARD_ECAT_SUPPORT_PORT2) && BOARD_ECAT_SUPPORT_PORT2
    gpio_write_pin(BOARD_ECAT_PHY2_RESET_GPIO,
                   BOARD_ECAT_PHY2_RESET_GPIO_PORT_INDEX,
                   BOARD_ECAT_PHY2_RESET_PIN_INDEX,
                   BOARD_ECAT_PHY_RESET_LEVEL);
#endif

    board_delay_ms(LAN8720_RESET_HOLD_TIME_MS);
}

hpm_stat_t ecat_phy_config(ESC_Type *ptr)
{
    (void) ptr;
    /*
     * LAN8720A usually works with hardware strap defaults in RMII mode.
     * Keep software configuration minimal here and allow board-level strap settings.
     */
    return status_success;
}


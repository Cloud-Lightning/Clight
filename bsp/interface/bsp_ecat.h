/**
 * @file bsp_ecat.h
 * @brief BSP EtherCAT abstraction interface
 */

#ifndef BSP_ECAT_H
#define BSP_ECAT_H

#include "bsp_common.h"
#include "bsp_enet.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BSP_ECAT_FEATURE_ESC              (1U << 0)
#define BSP_ECAT_FEATURE_MDIO             (1U << 1)
#define BSP_ECAT_FEATURE_EEPROM_EMULATION (1U << 2)
#define BSP_ECAT_FEATURE_IRQ              (1U << 3)

typedef enum {
    BSP_ECAT_PORT0 = 0,
    BSP_ECAT_PORT_MAX
} bsp_ecat_port_t;

typedef enum {
    BSP_ECAT_EEPROM_IDLE = 0,
    BSP_ECAT_EEPROM_READ = 1,
    BSP_ECAT_EEPROM_WRITE = 2,
    BSP_ECAT_EEPROM_RELOAD = 4
} bsp_ecat_eeprom_command_t;

typedef enum {
    BSP_ECAT_CTRL_SIGNAL_ALT_NMII_LINK0 = 0,
    BSP_ECAT_CTRL_SIGNAL_ALT_NMII_LINK1 = 1,
    BSP_ECAT_CTRL_SIGNAL_ALT_NMII_LINK2 = 2,
    BSP_ECAT_CTRL_SIGNAL_ALT_LINK_ACT0 = 3,
    BSP_ECAT_CTRL_SIGNAL_ALT_LINK_ACT1 = 4,
    BSP_ECAT_CTRL_SIGNAL_ALT_LINK_ACT2 = 5,
    BSP_ECAT_CTRL_SIGNAL_ALT_LED_RUN = 6,
    BSP_ECAT_CTRL_SIGNAL_ALT_LED_ERR = 7,
    BSP_ECAT_CTRL_SIGNAL_ALT_RESET_OUT = 8
} bsp_ecat_ctrl_signal_function_t;

typedef enum {
    BSP_ECAT_IRQ_NONE = 0U,
    BSP_ECAT_IRQ_SYNC0 = 1U << 0,
    BSP_ECAT_IRQ_SYNC1 = 1U << 1,
    BSP_ECAT_IRQ_RESET = 1U << 2,
    BSP_ECAT_IRQ_ALL = BSP_ECAT_IRQ_SYNC0 | BSP_ECAT_IRQ_SYNC1 | BSP_ECAT_IRQ_RESET
} bsp_ecat_irq_mask_t;

typedef struct {
    bool eeprom_emulation;
    bool eeprom_size_over_16kbit;
    bool core_clock_enable;
    bool phy_refclk_enable;
} bsp_ecat_eeprom_clock_config_t;

int32_t bsp_ecat_init(bsp_ecat_port_t port);
uintptr_t bsp_ecat_get_base_address(bsp_ecat_port_t port);
int32_t bsp_ecat_init_switch_led(bsp_ecat_port_t port);
int32_t bsp_ecat_init_eeprom_pins(bsp_ecat_port_t port);
int32_t bsp_ecat_init_eeprom_as_i2c_pins(bsp_ecat_port_t port);
int32_t bsp_ecat_set_mdio_owner(bsp_mdio_owner_t owner);
int32_t bsp_ecat_set_core_clock(bsp_ecat_port_t port, bool enable);
int32_t bsp_ecat_set_phy_clock(bsp_ecat_port_t port, bool enable);
int32_t bsp_ecat_config_eeprom_clock(bsp_ecat_port_t port, const bsp_ecat_eeprom_clock_config_t *config);
int32_t bsp_ecat_config_ctrl_signal(bsp_ecat_port_t port,
                                    uint8_t index,
                                    bsp_ecat_ctrl_signal_function_t function,
                                    bool invert);
int32_t bsp_ecat_config_nmii_link_source(bsp_ecat_port_t port,
                                         bool link0_from_io,
                                         bool link1_from_io,
                                         bool link2_from_io);
int32_t bsp_ecat_config_reset_source(bsp_ecat_port_t port, bool reset_from_ecat_core);
int32_t bsp_ecat_pdi_reset(bsp_ecat_port_t port);
int32_t bsp_ecat_set_phy_offset(bsp_ecat_port_t port, uint8_t offset);
int32_t bsp_ecat_mdio_read(bsp_ecat_port_t port, uint8_t phy_addr, uint8_t reg_addr, uint16_t *data);
int32_t bsp_ecat_mdio_write(bsp_ecat_port_t port, uint8_t phy_addr, uint8_t reg_addr, uint16_t data);
int32_t bsp_ecat_check_eeprom_loading(bsp_ecat_port_t port);
int32_t bsp_ecat_enable_irq(bsp_ecat_port_t port, bsp_ecat_irq_mask_t mask);
int32_t bsp_ecat_disable_irq(bsp_ecat_port_t port, bsp_ecat_irq_mask_t mask);
int32_t bsp_ecat_get_eeprom_command(bsp_ecat_port_t port, bsp_ecat_eeprom_command_t *command);
int32_t bsp_ecat_get_eeprom_byte_address(bsp_ecat_port_t port, uint32_t *address);
int32_t bsp_ecat_get_eeprom_word_address(bsp_ecat_port_t port, uint32_t *address);
int32_t bsp_ecat_read_eeprom_data(bsp_ecat_port_t port, uint64_t *data);
int32_t bsp_ecat_write_eeprom_data(bsp_ecat_port_t port, uint64_t data);
int32_t bsp_ecat_ack_eeprom_emulation(bsp_ecat_port_t port,
                                      bsp_ecat_eeprom_command_t command,
                                      bool ack_error,
                                      bool crc_error);

#ifdef __cplusplus
}
#endif

#endif /* BSP_ECAT_H */

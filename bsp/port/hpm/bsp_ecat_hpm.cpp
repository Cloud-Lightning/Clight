/**
 * @file bsp_ecat_hpm.cpp
 * @brief HPM platform implementation for BSP EtherCAT interface
 */

#include "bsp_ecat.h"

extern "C" {
#include "board.h"
#include "hpm_esc_drv.h"
}

static int32_t bsp_status_from_hpm(hpm_stat_t status)
{
    if (status == status_success) {
        return BSP_OK;
    }
    if (status == status_invalid_argument) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR;
}

static ESC_Type *bsp_ecat_get_base(bsp_ecat_port_t port)
{
    if (port != BSP_ECAT_PORT0) {
        return nullptr;
    }

#if defined(HPM_ESC)
    return HPM_ESC;
#else
    return nullptr;
#endif
}

static esc_ctrl_signal_function_t to_hpm_ctrl_signal_function(bsp_ecat_ctrl_signal_function_t function)
{
    switch (function) {
    case BSP_ECAT_CTRL_SIGNAL_ALT_NMII_LINK0:
        return esc_ctrl_signal_func_alt_nmii_link0;
    case BSP_ECAT_CTRL_SIGNAL_ALT_NMII_LINK1:
        return esc_ctrl_signal_func_alt_nmii_link1;
    case BSP_ECAT_CTRL_SIGNAL_ALT_NMII_LINK2:
        return esc_ctrl_signal_func_alt_nmii_link2;
    case BSP_ECAT_CTRL_SIGNAL_ALT_LINK_ACT0:
        return esc_ctrl_signal_func_alt_link_act0;
    case BSP_ECAT_CTRL_SIGNAL_ALT_LINK_ACT1:
        return esc_ctrl_signal_func_alt_link_act1;
    case BSP_ECAT_CTRL_SIGNAL_ALT_LINK_ACT2:
        return esc_ctrl_signal_func_alt_link_act2;
    case BSP_ECAT_CTRL_SIGNAL_ALT_LED_RUN:
        return esc_ctrl_signal_func_alt_led_run;
    case BSP_ECAT_CTRL_SIGNAL_ALT_LED_ERR:
        return esc_ctrl_signal_func_alt_led_err;
    case BSP_ECAT_CTRL_SIGNAL_ALT_RESET_OUT:
    default:
        return esc_ctrl_signal_func_alt_reset_out;
    }
}

static esc_irq_mask_t to_hpm_irq_mask(bsp_ecat_irq_mask_t mask)
{
    uint32_t value = esc_irq_mask_none;
    if ((mask & BSP_ECAT_IRQ_SYNC0) != 0U) {
        value |= esc_sync0_irq_mask;
    }
    if ((mask & BSP_ECAT_IRQ_SYNC1) != 0U) {
        value |= esc_sync1_irq_mask;
    }
    if ((mask & BSP_ECAT_IRQ_RESET) != 0U) {
        value |= esc_reset_irq_mask;
    }
    return static_cast<esc_irq_mask_t>(value);
}

static esc_eeprom_cmd_t to_hpm_eeprom_command(bsp_ecat_eeprom_command_t command)
{
    switch (command) {
    case BSP_ECAT_EEPROM_READ:
        return esc_eeprom_read_cmd;
    case BSP_ECAT_EEPROM_WRITE:
        return esc_eeprom_write_cmd;
    case BSP_ECAT_EEPROM_RELOAD:
        return esc_eeprom_reload_cmd;
    case BSP_ECAT_EEPROM_IDLE:
    default:
        return esc_eeprom_idle_cmd;
    }
}

static bsp_ecat_eeprom_command_t from_hpm_eeprom_command(uint8_t command)
{
    switch (command) {
    case esc_eeprom_read_cmd:
        return BSP_ECAT_EEPROM_READ;
    case esc_eeprom_write_cmd:
        return BSP_ECAT_EEPROM_WRITE;
    case esc_eeprom_reload_cmd:
        return BSP_ECAT_EEPROM_RELOAD;
    case esc_eeprom_idle_cmd:
    default:
        return BSP_ECAT_EEPROM_IDLE;
    }
}

int32_t bsp_ecat_init(bsp_ecat_port_t port)
{
    auto *base = bsp_ecat_get_base(port);
    if (base == nullptr) {
        return BSP_ERROR_PARAM;
    }

    board_init_ethercat(base);
    return BSP_OK;
}

uintptr_t bsp_ecat_get_base_address(bsp_ecat_port_t port)
{
    return reinterpret_cast<uintptr_t>(bsp_ecat_get_base(port));
}

int32_t bsp_ecat_init_switch_led(bsp_ecat_port_t port)
{
    if (bsp_ecat_get_base(port) == nullptr) {
        return BSP_ERROR_PARAM;
    }

    board_init_switch_led();
    return BSP_OK;
}

int32_t bsp_ecat_init_eeprom_pins(bsp_ecat_port_t port)
{
    if (bsp_ecat_get_base(port) == nullptr) {
        return BSP_ERROR_PARAM;
    }
    init_esc_eeprom_pin();
    return BSP_OK;
}

int32_t bsp_ecat_init_eeprom_as_i2c_pins(bsp_ecat_port_t port)
{
    if (bsp_ecat_get_base(port) == nullptr) {
        return BSP_ERROR_PARAM;
    }
    init_esc_eeprom_as_i2c_pin();
    return BSP_OK;
}

int32_t bsp_ecat_set_mdio_owner(bsp_mdio_owner_t owner)
{
    if (owner == BSP_MDIO_OWNER_ESC) {
        board_select_mdio_owner_for_esc();
        return BSP_OK;
    }

    if (owner == BSP_MDIO_OWNER_ENET) {
        board_select_mdio_owner_for_enet();
        return BSP_OK;
    }

    return BSP_ERROR_PARAM;
}

int32_t bsp_ecat_set_core_clock(bsp_ecat_port_t port, bool enable)
{
    auto *base = bsp_ecat_get_base(port);
    if (base == nullptr) {
        return BSP_ERROR_PARAM;
    }
    esc_core_enable_clock(base, enable);
    return BSP_OK;
}

int32_t bsp_ecat_set_phy_clock(bsp_ecat_port_t port, bool enable)
{
    auto *base = bsp_ecat_get_base(port);
    if (base == nullptr) {
        return BSP_ERROR_PARAM;
    }
    esc_phy_enable_clock(base, enable);
    return BSP_OK;
}

int32_t bsp_ecat_config_eeprom_clock(bsp_ecat_port_t port, const bsp_ecat_eeprom_clock_config_t *config)
{
    auto *base = bsp_ecat_get_base(port);
    if ((base == nullptr) || (config == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    esc_eeprom_clock_config_t local = {
        .eeprom_emulation = config->eeprom_emulation,
        .eeprom_size_over_16kbit = config->eeprom_size_over_16kbit,
        .core_clock_en = config->core_clock_enable,
        .phy_refclk_en = config->phy_refclk_enable,
    };
    esc_config_eeprom_and_clock(base, &local);
    return BSP_OK;
}

int32_t bsp_ecat_config_ctrl_signal(bsp_ecat_port_t port,
                                    uint8_t index,
                                    bsp_ecat_ctrl_signal_function_t function,
                                    bool invert)
{
    auto *base = bsp_ecat_get_base(port);
    if (base == nullptr) {
        return BSP_ERROR_PARAM;
    }
    esc_config_ctrl_signal_function(base, index, to_hpm_ctrl_signal_function(function), invert);
    return BSP_OK;
}

int32_t bsp_ecat_config_nmii_link_source(bsp_ecat_port_t port,
                                         bool link0_from_io,
                                         bool link1_from_io,
                                         bool link2_from_io)
{
    auto *base = bsp_ecat_get_base(port);
    if (base == nullptr) {
        return BSP_ERROR_PARAM;
    }
    esc_config_nmii_link_source(base, link0_from_io, link1_from_io, link2_from_io);
    return BSP_OK;
}

int32_t bsp_ecat_config_reset_source(bsp_ecat_port_t port, bool reset_from_ecat_core)
{
    auto *base = bsp_ecat_get_base(port);
    if (base == nullptr) {
        return BSP_ERROR_PARAM;
    }
    esc_config_reset_source(base, reset_from_ecat_core);
    return BSP_OK;
}

int32_t bsp_ecat_pdi_reset(bsp_ecat_port_t port)
{
    auto *base = bsp_ecat_get_base(port);
    if (base == nullptr) {
        return BSP_ERROR_PARAM;
    }
    esc_pdi_reset(base);
    return BSP_OK;
}

int32_t bsp_ecat_set_phy_offset(bsp_ecat_port_t port, uint8_t offset)
{
    auto *base = bsp_ecat_get_base(port);
    if (base == nullptr) {
        return BSP_ERROR_PARAM;
    }
    esc_set_phy_offset(base, offset);
    return BSP_OK;
}

int32_t bsp_ecat_mdio_read(bsp_ecat_port_t port, uint8_t phy_addr, uint8_t reg_addr, uint16_t *data)
{
    auto *base = bsp_ecat_get_base(port);
    if ((base == nullptr) || (data == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    return bsp_status_from_hpm(esc_mdio_read(base, phy_addr, reg_addr, data));
}

int32_t bsp_ecat_mdio_write(bsp_ecat_port_t port, uint8_t phy_addr, uint8_t reg_addr, uint16_t data)
{
    auto *base = bsp_ecat_get_base(port);
    if (base == nullptr) {
        return BSP_ERROR_PARAM;
    }
    return bsp_status_from_hpm(esc_mdio_write(base, phy_addr, reg_addr, data));
}

int32_t bsp_ecat_check_eeprom_loading(bsp_ecat_port_t port)
{
    auto *base = bsp_ecat_get_base(port);
    if (base == nullptr) {
        return BSP_ERROR_PARAM;
    }
    return bsp_status_from_hpm(esc_check_eeprom_loading(base));
}

int32_t bsp_ecat_enable_irq(bsp_ecat_port_t port, bsp_ecat_irq_mask_t mask)
{
    auto *base = bsp_ecat_get_base(port);
    if (base == nullptr) {
        return BSP_ERROR_PARAM;
    }
    esc_enable_irq(base, to_hpm_irq_mask(mask));
    return BSP_OK;
}

int32_t bsp_ecat_disable_irq(bsp_ecat_port_t port, bsp_ecat_irq_mask_t mask)
{
    auto *base = bsp_ecat_get_base(port);
    if (base == nullptr) {
        return BSP_ERROR_PARAM;
    }
    esc_disable_irq(base, to_hpm_irq_mask(mask));
    return BSP_OK;
}

int32_t bsp_ecat_get_eeprom_command(bsp_ecat_port_t port, bsp_ecat_eeprom_command_t *command)
{
    auto *base = bsp_ecat_get_base(port);
    if ((base == nullptr) || (command == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    *command = from_hpm_eeprom_command(esc_get_eeprom_cmd(base));
    return BSP_OK;
}

int32_t bsp_ecat_get_eeprom_byte_address(bsp_ecat_port_t port, uint32_t *address)
{
    auto *base = bsp_ecat_get_base(port);
    if ((base == nullptr) || (address == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    *address = esc_get_eeprom_byte_address(base);
    return BSP_OK;
}

int32_t bsp_ecat_get_eeprom_word_address(bsp_ecat_port_t port, uint32_t *address)
{
    auto *base = bsp_ecat_get_base(port);
    if ((base == nullptr) || (address == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    *address = esc_get_eeprom_word_address(base);
    return BSP_OK;
}

int32_t bsp_ecat_read_eeprom_data(bsp_ecat_port_t port, uint64_t *data)
{
    auto *base = bsp_ecat_get_base(port);
    if ((base == nullptr) || (data == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    *data = esc_read_eeprom_data(base);
    return BSP_OK;
}

int32_t bsp_ecat_write_eeprom_data(bsp_ecat_port_t port, uint64_t data)
{
    auto *base = bsp_ecat_get_base(port);
    if (base == nullptr) {
        return BSP_ERROR_PARAM;
    }
    esc_write_eeprom_data(base, data);
    return BSP_OK;
}

int32_t bsp_ecat_ack_eeprom_emulation(bsp_ecat_port_t port,
                                      bsp_ecat_eeprom_command_t command,
                                      bool ack_error,
                                      bool crc_error)
{
    auto *base = bsp_ecat_get_base(port);
    if (base == nullptr) {
        return BSP_ERROR_PARAM;
    }
    esc_eeprom_emulation_ack(base, to_hpm_eeprom_command(command), ack_error, crc_error);
    return BSP_OK;
}

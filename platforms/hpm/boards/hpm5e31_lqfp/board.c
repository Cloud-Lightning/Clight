/*
 * Copyright (c) 2024 HPMicro
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "board.h"
#include "clock.h"
#include "pinmux.h"
#include "hpm_clock_drv.h"
#include "hpm_i2c_drv.h"
#include "hpm_gpio_drv.h"
#include "hpm_gptmr_drv.h"
#include "hpm_mcan_drv.h"
#include "hpm_pllctlv2_drv.h"
#include "hpm_enet_drv.h"
#include "hpm_spi_drv.h"
#include "hpm_interrupt.h"
#include "hpm_esc_drv.h"
#include "hpm_uart_drv.h"
#include "hpm_usb_drv.h"
#include "hpm_sdk_version.h"
#include "bsp_gptmr.h"
#include <stddef.h>
#include <stdio.h>

/**
 * @brief FLASH configuration option definitions:
 * option[0]:
 *    [31:16] 0xfcf9 - FLASH configuration option tag
 *    [15:4]  0 - Reserved
 *    [3:0]   option words (exclude option[0])
 * option[1]:
 *    [31:28] Flash probe type
 *      0 - SFDP SDR / 1 - SFDP DDR
 *      2 - 1-4-4 Read (0xEB, 24-bit address) / 3 - 1-2-2 Read(0xBB, 24-bit address)
 *      4 - HyperFLASH 1.8V / 5 - HyperFLASH 3V
 *      6 - OctaBus DDR (SPI -> OPI DDR)
 *      8 - Xccela DDR (SPI -> OPI DDR)
 *      10 - EcoXiP DDR (SPI -> OPI DDR)
 *    [27:24] Command Pads after Power-on Reset
 *      0 - SPI / 1 - DPI / 2 - QPI / 3 - OPI
 *    [23:20] Command Pads after Configuring FLASH
 *      0 - SPI / 1 - DPI / 2 - QPI / 3 - OPI
 *    [19:16] Quad Enable Sequence (for the device support SFDP 1.0 only)
 *      0 - Not needed
 *      1 - QE bit is at bit 6 in Status Register 1
 *      2 - QE bit is at bit1 in Status Register 2
 *      3 - QE bit is at bit7 in Status Register 2
 *      4 - QE bit is at bit1 in Status Register 2 and should be programmed by 0x31
 *    [15:8] Dummy cycles
 *      0 - Auto-probed / detected / default value
 *      Others - User specified value, for DDR read, the dummy cycles should be 2 * cycles on FLASH datasheet
 *    [7:4] Misc.
 *      0 - Not used
 *      1 - SPI mode
 *      2 - Internal loopback
 *      3 - External DQS
 *    [3:0] Frequency option
 *      1 - 30MHz / 2 - 50MHz / 3 - 66MHz / 4 - 80MHz / 5 - 100MHz / 6 - 120MHz / 7 - 133MHz / 8 - 166MHz
 *
 * option[2] (Effective only if the bit[3:0] in option[0] > 1)
 *    [31:20]  Reserved
 *    [19:16] IO voltage
 *      0 - 3V / 1 - 1.8V
 *    [15:12] Pin group
 *      0 - 1st group / 1 - 2nd group
 *    [11:8] Connection selection
 *      0 - CA_CS0 / 1 - CB_CS0 / 2 - CA_CS0 + CB_CS0 (Two FLASH connected to CA and CB respectively)
 *    [7:0] Drive Strength
 *      0 - Default value
 * option[3] (Effective only if the bit[3:0] in option[0] > 2, required only for the QSPI NOR FLASH that not supports
 *              JESD216)
 *    [31:16] reserved
 *    [15:12] Sector Erase Command Option, not required here
 *    [11:8]  Sector Size Option, not required here
 *    [7:0] Flash Size Option
 *      0 - 4MB / 1 - 8MB / 2 - 16MB
 */
#if defined(FLASH_XIP) && FLASH_XIP
/* __attribute__ ((section(".nor_cfg_option"), used)) const uint32_t option[4] = {0xfcf90002, 0x00000007, 0xE, 0x0}; */    /* setting by user's board */
#endif

#if defined(FLASH_UF2) && FLASH_UF2
ATTR_PLACE_AT(".uf2_signature") __attribute__((used)) const uint32_t uf2_signature = BOARD_UF2_SIGNATURE;
#endif

static board_timer_cb s_board_timer_cb;

void board_init_clock(void)
{
    init_clocks();
    clock_add_to_group(clock_cpu0, 0);
    clock_add_to_group(clock_mchtmr0, 0);
    clock_add_to_group(clock_ahb0, 0);
    clock_add_to_group(clock_axif, 0);
    clock_add_to_group(clock_axis, 0);
    clock_add_to_group(clock_axic, 0);
    clock_add_to_group(clock_gpio, 0);
    clock_connect_group_to_cpu(0, 0);
}

void board_init_console(void)
{
#if !defined(CONFIG_NDEBUG_CONSOLE) || !CONFIG_NDEBUG_CONSOLE
#if BOARD_CONSOLE_TYPE == CONSOLE_TYPE_UART
    console_config_t cfg;

    clock_add_to_group(BOARD_CONSOLE_UART_CLK_NAME, 0);

    cfg.type = BOARD_CONSOLE_TYPE;
    cfg.base = (uint32_t) BOARD_CONSOLE_UART_BASE;
    cfg.src_freq_in_hz = clock_get_frequency(BOARD_CONSOLE_UART_CLK_NAME);
    cfg.baudrate = BOARD_CONSOLE_UART_BAUDRATE;

    if (status_success != console_init(&cfg)) {
        while (1) {
        }
    }
#else
    while (1) {
    }
#endif
#endif
}

void board_print_banner(void)
{
    const uint8_t banner[] = "\n"
"----------------------------------------------------------------------\n"
"$$\\   $$\\ $$$$$$$\\  $$\\      $$\\ $$\\\n"
"$$ |  $$ |$$  __$$\\ $$$\\    $$$ |\\__|\n"
"$$ |  $$ |$$ |  $$ |$$$$\\  $$$$ |$$\\  $$$$$$$\\  $$$$$$\\   $$$$$$\\\n"
"$$$$$$$$ |$$$$$$$  |$$\\$$\\$$ $$ |$$ |$$  _____|$$  __$$\\ $$  __$$\\\n"
"$$  __$$ |$$  ____/ $$ \\$$$  $$ |$$ |$$ /      $$ |  \\__|$$ /  $$ |\n"
"$$ |  $$ |$$ |      $$ |\\$  /$$ |$$ |$$ |      $$ |      $$ |  $$ |\n"
"$$ |  $$ |$$ |      $$ | \\_/ $$ |$$ |\\$$$$$$$\\ $$ |      \\$$$$$$  |\n"
"\\__|  \\__|\\__|      \\__|     \\__|\\__| \\_______|\\__|       \\______/\n"
"----------------------------------------------------------------------\n";
#ifdef SDK_VERSION_STRING
    printf("hpm_sdk: %s\n", SDK_VERSION_STRING);
#endif
    printf("%s", banner);
}

uint32_t board_init_uart_clock(UART_Type *ptr)
{
    uint32_t freq = 0U;

    if (ptr == HPM_UART0) {
        clock_add_to_group(clock_uart0, BOARD_RUNNING_CORE & 0x1);
        freq = clock_get_frequency(clock_uart0);
    } else if (ptr == HPM_UART3) {
        clock_add_to_group(clock_uart3, BOARD_RUNNING_CORE & 0x1);
        freq = clock_get_frequency(clock_uart3);
    }

    return freq;
}

void board_init_uart(UART_Type *ptr)
{
    init_uart_pins(ptr);
    (void) board_init_uart_clock(ptr);
}

void board_print_clock_freq(void)
{
    printf("==============================\n");
    printf(" %s clock summary\n", BOARD_NAME);
    printf("==============================\n");
    printf("cpu0:\t\t %luHz\n", clock_get_frequency(clock_cpu0));
    printf("ahb:\t\t %luHz\n", clock_get_frequency(clock_ahb0));
    printf("mchtmr0:\t %luHz\n", clock_get_frequency(clock_mchtmr0));
    printf("xpi0:\t\t %luHz\n", clock_get_frequency(clock_xpi0));
    printf("==============================\n");
}

uint32_t board_init_i2c_clock(I2C_Type *ptr)
{
    uint32_t freq = 0U;

    if (ptr == HPM_I2C3) {
        clock_add_to_group(clock_i2c3, BOARD_RUNNING_CORE & 0x1);
        freq = clock_get_frequency(clock_i2c3);
    }

    return freq;
}

void board_init_i2c(I2C_Type *ptr)
{
    i2c_config_t config = {0};
    uint32_t freq = board_init_i2c_clock(ptr);

    init_i2c_pins(ptr);

    config.i2c_mode = i2c_mode_normal;
    config.is_10bit_addressing = false;
    (void) i2c_init_master(ptr, freq, &config);
}

uint32_t board_init_spi_clock(SPI_Type *ptr)
{
    if (ptr == HPM_SPI1) {
        clock_add_to_group(clock_spi1, BOARD_RUNNING_CORE & 0x1);
        return clock_get_frequency(clock_spi1);
    }

    return 0U;
}

void board_init_spi_pins(SPI_Type *ptr)
{
    (void) ptr;
    init_spi_pins();
}

void board_init_spi_pins_with_gpio_as_cs(SPI_Type *ptr)
{
    board_init_spi_pins(ptr);
}

void board_init_adc16_pins(void)
{
    init_adc16_pins();
}

uint32_t board_init_adc_clock(void *ptr, bool clk_src_bus)
{
    uint32_t freq = 0U;

    if (ptr == (void *) HPM_ADC0) {
        clock_add_to_group(clock_adc0, BOARD_RUNNING_CORE & 0x1);
        clock_set_adc_source(clock_adc0, clk_src_bus ? clk_adc_src_ahb0 : clk_adc_src_ana0);
        freq = clock_get_frequency(clock_adc0);
    } else if (ptr == (void *) HPM_ADC1) {
        clock_add_to_group(clock_adc1, BOARD_RUNNING_CORE & 0x1);
        clock_set_adc_source(clock_adc1, clk_src_bus ? clk_adc_src_ahb0 : clk_adc_src_ana0);
        freq = clock_get_frequency(clock_adc1);
    }

    return freq;
}

void board_init_can(MCAN_Type *ptr)
{
    init_can_pins(ptr);
}

uint32_t board_init_can_clock(MCAN_Type *ptr)
{
    uint32_t freq = 0U;

    if (ptr == HPM_MCAN2) {
        clock_add_to_group(clock_can2, BOARD_RUNNING_CORE & 0x1);
        clock_set_source_divider(clock_can2, clk_src_pll1_clk0, 10U);
        freq = clock_get_frequency(clock_can2);
    }

    return freq;
}

uint32_t board_init_gptmr_clock(GPTMR_Type *ptr)
{
    uint32_t freq = 0U;

    if (ptr == HPM_GPTMR0) {
        clock_add_to_group(clock_gptmr0, BOARD_RUNNING_CORE & 0x1);
        freq = clock_get_frequency(clock_gptmr0);
    } else if (ptr == HPM_GPTMR1) {
        clock_add_to_group(clock_gptmr1, BOARD_RUNNING_CORE & 0x1);
        freq = clock_get_frequency(clock_gptmr1);
    } else if (ptr == HPM_GPTMR2) {
        clock_add_to_group(clock_gptmr2, BOARD_RUNNING_CORE & 0x1);
        freq = clock_get_frequency(clock_gptmr2);
    } else if (ptr == HPM_GPTMR3) {
        clock_add_to_group(clock_gptmr3, BOARD_RUNNING_CORE & 0x1);
        freq = clock_get_frequency(clock_gptmr3);
    }

    return freq;
}

void board_init_gptmr_channel_pin(GPTMR_Type *ptr, uint32_t channel, bool as_output)
{
    (void) channel;
    (void) as_output;
    init_gptmr_pins(ptr);
}

static void board_configure_enet_rgmii_pins(void)
{
    HPM_IOC->PAD[IOC_PAD_PF02].FUNC_CTL = IOC_PF02_FUNC_CTL_ETH0_TXCK;
    HPM_IOC->PAD[IOC_PAD_PF03].FUNC_CTL = IOC_PF03_FUNC_CTL_ETH0_TXD_0;
    HPM_IOC->PAD[IOC_PAD_PF04].FUNC_CTL = IOC_PF04_FUNC_CTL_ETH0_TXD_1;
    HPM_IOC->PAD[IOC_PAD_PF05].FUNC_CTL = IOC_PF05_FUNC_CTL_ETH0_TXD_2;
    HPM_IOC->PAD[IOC_PAD_PF06].FUNC_CTL = IOC_PF06_FUNC_CTL_ETH0_TXD_3;
    HPM_IOC->PAD[IOC_PAD_PF07].FUNC_CTL = IOC_PF07_FUNC_CTL_ETH0_TXEN;
    HPM_IOC->PAD[IOC_PAD_PF08].FUNC_CTL = IOC_PF08_FUNC_CTL_ETH0_RXDV;
    HPM_IOC->PAD[IOC_PAD_PF09].FUNC_CTL = IOC_PF09_FUNC_CTL_ETH0_RXD_0;
    HPM_IOC->PAD[IOC_PAD_PF10].FUNC_CTL = IOC_PF10_FUNC_CTL_ETH0_RXD_1;
    HPM_IOC->PAD[IOC_PAD_PF11].FUNC_CTL = IOC_PF11_FUNC_CTL_ETH0_RXD_2;
    HPM_IOC->PAD[IOC_PAD_PF12].FUNC_CTL = IOC_PF12_FUNC_CTL_ETH0_RXD_3;
    HPM_IOC->PAD[IOC_PAD_PF13].FUNC_CTL = IOC_PF13_FUNC_CTL_ETH0_RXCK;
}

void board_select_mdio_owner_for_enet(void)
{
    HPM_IOC->PAD[IOC_PAD_PA30].FUNC_CTL = IOC_PA30_FUNC_CTL_ETH0_MDIO;
    HPM_IOC->PAD[IOC_PAD_PA31].FUNC_CTL = IOC_PA31_FUNC_CTL_ETH0_MDC;
}

void board_select_mdio_owner_for_esc(void)
{
    HPM_IOC->PAD[IOC_PAD_PA30].FUNC_CTL = IOC_PA30_FUNC_CTL_ESC0_MDIO;
    HPM_IOC->PAD[IOC_PAD_PA31].FUNC_CTL = IOC_PA31_FUNC_CTL_ESC0_MDC;
}

void board_init(void)
{
    board_init_clock();
    board_init_gpio_pins();
    board_init_console();
#if BOARD_SHOW_CLOCK
    board_print_clock_freq();
#endif
#if BOARD_SHOW_BANNER
    board_print_banner();
#endif
}

void board_init_core1(void)
{
}

void board_init_gpio_pins(void)
{
    init_pins();
    gpio_set_pin_input(BOARD_APP_GPIO_CTRL, BOARD_APP_GPIO_INDEX, BOARD_APP_GPIO_PIN);
}

void board_init_led_pins(void)
{
    init_led_pins();
    gpio_set_pin_output_with_initial(BOARD_LED_GPIO_CTRL,
                                     BOARD_LED_GPIO_INDEX,
                                     BOARD_LED_GPIO_PIN,
                                     BOARD_LED_OFF_LEVEL);
}

void board_led_write(uint8_t state)
{
    gpio_write_pin(BOARD_LED_GPIO_CTRL,
                   BOARD_LED_GPIO_INDEX,
                   BOARD_LED_GPIO_PIN,
                   state ? BOARD_LED_ON_LEVEL : BOARD_LED_OFF_LEVEL);
}

SDK_DECLARE_EXT_ISR_M(BOARD_CALLBACK_TIMER_IRQ, board_timer_isr);
void bsp_gptmr_irq_handler(bsp_gptmr_instance_t timer);
void bsp_hpm_gptmr_timer_notify_from_board(void);
void board_timer_isr(void)
{
    if (gptmr_check_status(BOARD_CALLBACK_TIMER, GPTMR_CH_RLD_STAT_MASK(BOARD_CALLBACK_TIMER_CH))) {
        gptmr_clear_status(BOARD_CALLBACK_TIMER, GPTMR_CH_RLD_STAT_MASK(BOARD_CALLBACK_TIMER_CH));
        if (s_board_timer_cb != NULL) {
            s_board_timer_cb();
        }
        bsp_hpm_gptmr_timer_notify_from_board();
    }

    if (gptmr_check_status(BOARD_CALLBACK_TIMER, GPTMR_CH_CMP_STAT_MASK(0U, 0U))) {
        gptmr_clear_status(BOARD_CALLBACK_TIMER, GPTMR_CH_CMP_STAT_MASK(0U, 0U));
        bsp_gptmr_irq_handler(BSP_GPTMR_COMPARE);
    }
}

void board_timer_create(uint32_t ms, board_timer_cb cb)
{
    uint32_t gptmr_freq;
    gptmr_channel_config_t config;

    s_board_timer_cb = cb;
    gptmr_channel_get_default_config(BOARD_CALLBACK_TIMER, &config);

    clock_add_to_group(BOARD_CALLBACK_TIMER_CLK_NAME, BOARD_RUNNING_CORE & 0x1);
    gptmr_freq = clock_get_frequency(BOARD_CALLBACK_TIMER_CLK_NAME);

    config.reload = (gptmr_freq / 1000U) * ms;
    gptmr_channel_config(BOARD_CALLBACK_TIMER, BOARD_CALLBACK_TIMER_CH, &config, false);
    gptmr_enable_irq(BOARD_CALLBACK_TIMER, GPTMR_CH_RLD_IRQ_MASK(BOARD_CALLBACK_TIMER_CH));
    intc_m_enable_irq_with_priority(BOARD_CALLBACK_TIMER_IRQ, 1);
    gptmr_start_counter(BOARD_CALLBACK_TIMER, BOARD_CALLBACK_TIMER_CH);
}

void board_delay_ms(uint32_t ms)
{
    clock_cpu_delay_ms(ms);
}

uint8_t board_get_led_gpio_off_level(void)
{
    return BOARD_LED_OFF_LEVEL;
}

void board_init_usb(USB_Type *ptr)
{
    if (ptr == HPM_USB0) {
        clock_add_to_group(clock_usb0, BOARD_RUNNING_CORE & 0x1);
        usb_phy_using_internal_vbus(ptr);
    }
}

hpm_stat_t board_init_enet_pins(ENET_Type *ptr)
{
    if (ptr != BOARD_ENET_RGMII) {
        return status_invalid_argument;
    }

    board_configure_enet_rgmii_pins();
    board_select_mdio_owner_for_enet();

    HPM_IOC->PAD[IOC_PAD_PA21].FUNC_CTL = IOC_PA21_FUNC_CTL_GPIO_A_21;
    gpio_set_pin_output_with_initial(BOARD_ENET_RGMII_RST_GPIO,
                                     BOARD_ENET_RGMII_RST_GPIO_INDEX,
                                     BOARD_ENET_RGMII_RST_GPIO_PIN,
                                     0U);
    return status_success;
}

hpm_stat_t board_reset_enet_phy(ENET_Type *ptr)
{
    if (ptr != BOARD_ENET_RGMII) {
        return status_invalid_argument;
    }

    gpio_write_pin(BOARD_ENET_RGMII_RST_GPIO, BOARD_ENET_RGMII_RST_GPIO_INDEX, BOARD_ENET_RGMII_RST_GPIO_PIN, 0U);
    /* RTL8211F datasheet requires PHYRSTB low for at least 10 ms. */
    board_delay_ms(10U);
    gpio_write_pin(BOARD_ENET_RGMII_RST_GPIO, BOARD_ENET_RGMII_RST_GPIO_INDEX, BOARD_ENET_RGMII_RST_GPIO_PIN, 1U);
    board_delay_ms(50U);
    return status_success;
}

uint8_t board_get_enet_dma_pbl(ENET_Type *ptr)
{
    (void) ptr;
    return enet_pbl_32;
}

hpm_stat_t board_init_enet_ptp_clock(ENET_Type *ptr)
{
    if (ptr != BOARD_ENET_RGMII) {
        return status_invalid_argument;
    }

    clock_add_to_group(clock_ptp0, BOARD_RUNNING_CORE & 0x1);
    clock_set_source_divider(clock_ptp0, clk_src_pll1_clk0, 4U);
    return status_success;
}

hpm_stat_t board_init_enet_rgmii_clock_delay(ENET_Type *ptr)
{
    if (ptr != BOARD_ENET_RGMII) {
        return status_invalid_argument;
    }

    clock_add_to_group(clock_eth0, BOARD_RUNNING_CORE & 0x1);
    return enet_rgmii_set_clock_delay(ptr, BOARD_ENET_RGMII_TX_DLY, BOARD_ENET_RGMII_RX_DLY);
}

hpm_stat_t board_init_enet_rmii_reference_clock(ENET_Type *ptr, bool internal)
{
    if (ptr != BOARD_ENET_RGMII) {
        return status_invalid_argument;
    }

    clock_add_to_group(clock_eth0, BOARD_RUNNING_CORE & 0x1);
    clock_add_to_group(clock_esc0, BOARD_RUNNING_CORE & 0x1);
    esc_core_enable_clock(HPM_ESC, true);
    esc_phy_enable_clock(HPM_ESC, true);

    if (internal) {
        if (pllctlv2_init_pll_with_freq(HPM_PLLCTLV2, pllctlv2_pll2, 500000000UL) != status_success) {
            return status_fail;
        }
        pllctlv2_set_postdiv(HPM_PLLCTLV2, pllctlv2_pll2, pllctlv2_clk1, pllctlv2_div_2p0);
        clock_set_source_divider(clock_eth0, clk_src_pll2_clk1, 5U);
    }

    enet_rmii_enable_clock(ptr, internal);
    return status_success;
}

hpm_stat_t board_init_enet_mii_clock(ENET_Type *ptr)
{
    if (ptr != BOARD_ENET_RGMII) {
        return status_invalid_argument;
    }

    clock_add_to_group(clock_eth0, BOARD_RUNNING_CORE & 0x1);
    clock_add_to_group(clock_esc0, BOARD_RUNNING_CORE & 0x1);
    esc_core_enable_clock(HPM_ESC, true);
    esc_phy_enable_clock(HPM_ESC, true);
    return status_success;
}

hpm_stat_t board_enable_enet_irq(ENET_Type *ptr)
{
    if (ptr != BOARD_ENET_RGMII) {
        return status_invalid_argument;
    }

    intc_m_enable_irq(IRQn_ENET0);
    return status_success;
}

hpm_stat_t board_disable_enet_irq(ENET_Type *ptr)
{
    if (ptr != BOARD_ENET_RGMII) {
        return status_invalid_argument;
    }

    intc_m_disable_irq(IRQn_ENET0);
    return status_success;
}

ENET_Type *board_get_enet_base(uint8_t idx)
{
    if (idx == 0U) {
        return BOARD_ENET_RGMII;
    }

    return NULL;
}

void board_init_ethercat(ESC_Type *ptr)
{
    (void) ptr;

    clock_add_to_group(clock_esc0, BOARD_RUNNING_CORE & 0x1);
    init_esc_pins();

    gpio_set_pin_output_with_initial(BOARD_ECAT_PHY0_RESET_GPIO,
                                     BOARD_ECAT_PHY0_RESET_GPIO_PORT_INDEX,
                                     BOARD_ECAT_PHY0_RESET_PIN_INDEX,
                                     BOARD_ECAT_PHY_RESET_LEVEL);
}

void board_init_switch_led(void)
{
    init_esc_in_out_pin();

    gpio_set_pin_input(BOARD_ECAT_IN1_GPIO,
                       BOARD_ECAT_IN1_GPIO_PORT_INDEX,
                       BOARD_ECAT_IN1_GPIO_PIN_INDEX);
    gpio_set_pin_input(BOARD_ECAT_IN2_GPIO,
                       BOARD_ECAT_IN2_GPIO_PORT_INDEX,
                       BOARD_ECAT_IN2_GPIO_PIN_INDEX);

    gpio_set_pin_output_with_initial(BOARD_ECAT_OUT1_GPIO,
                                     BOARD_ECAT_OUT1_GPIO_PORT_INDEX,
                                     BOARD_ECAT_OUT1_GPIO_PIN_INDEX,
                                     !BOARD_ECAT_OUT_ON_LEVEL);
    gpio_set_pin_output_with_initial(BOARD_ECAT_OUT2_GPIO,
                                     BOARD_ECAT_OUT2_GPIO_PORT_INDEX,
                                     BOARD_ECAT_OUT2_GPIO_PIN_INDEX,
                                     !BOARD_ECAT_OUT_ON_LEVEL);
    gpio_set_pin_output_with_initial(BOARD_ECAT_OUT3_GPIO,
                                     BOARD_ECAT_OUT3_GPIO_PORT_INDEX,
                                     BOARD_ECAT_OUT3_GPIO_PIN_INDEX,
                                     !BOARD_ECAT_OUT_ON_LEVEL);
    gpio_set_pin_output_with_initial(BOARD_ECAT_OUT4_GPIO,
                                     BOARD_ECAT_OUT4_GPIO_PORT_INDEX,
                                     BOARD_ECAT_OUT4_GPIO_PIN_INDEX,
                                     !BOARD_ECAT_OUT_ON_LEVEL);
}

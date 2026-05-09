/*
 * Copyright (c) 2024 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _HPM_BOARD_H
#define _HPM_BOARD_H

#include "hpm_common.h"
#include "hpm_soc.h"
#include "hpm_clock_drv.h"
#include "hpm_enet_drv.h"
#include "hpm_esc_drv.h"
#include "hpm_pwmv2_drv.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#if !defined(CONFIG_NDEBUG_CONSOLE) || !CONFIG_NDEBUG_CONSOLE
#include "hpm_debug_console.h"
#endif

#define BOARD_NAME          "hpm5e31_lqfp"
#define BOARD_UF2_SIGNATURE (0x0A4D5048UL)

#define SEC_CORE_IMG_START ILM_LOCAL_BASE

#ifndef BOARD_RUNNING_CORE
#define BOARD_RUNNING_CORE HPM_CORE0
#endif

#if !defined(CONFIG_NDEBUG_CONSOLE) || !CONFIG_NDEBUG_CONSOLE
#ifndef BOARD_CONSOLE_TYPE
#define BOARD_CONSOLE_TYPE CONSOLE_TYPE_UART
#endif

#if BOARD_CONSOLE_TYPE == CONSOLE_TYPE_UART
#ifndef BOARD_CONSOLE_UART_BASE
#define BOARD_CONSOLE_UART_BASE       HPM_UART0
#define BOARD_CONSOLE_UART_CLK_NAME   clock_uart0
#define BOARD_CONSOLE_UART_IRQ        IRQn_UART0
#endif
#define BOARD_CONSOLE_UART_BAUDRATE   (115200UL)
#endif
#endif

#ifndef BOARD_SHOW_CLOCK
#define BOARD_SHOW_CLOCK 1
#endif
#ifndef BOARD_SHOW_BANNER
#define BOARD_SHOW_BANNER 1
#endif

/* GPIO example related board definitions */
#define BOARD_APP_GPIO_CTRL HPM_GPIO0
#define BOARD_APP_GPIO_INDEX GPIO_DI_GPIOF
#define BOARD_APP_GPIO_PIN   19U
#define BOARD_APP_GPIO_IRQ   IRQn_GPIO0_F
#define BOARD_BUTTON_PRESSED_VALUE 1U

#define BOARD_LED_GPIO_CTRL HPM_GPIO0
#define BOARD_LED_GPIO_INDEX GPIO_DI_GPIOA
#define BOARD_LED_GPIO_PIN   3U
#define BOARD_LED_OFF_LEVEL  0U
#define BOARD_LED_ON_LEVEL   1U

/* board timer */
#define BOARD_CALLBACK_TIMER          (HPM_GPTMR3)
#define BOARD_CALLBACK_TIMER_CH       (1U)
#define BOARD_CALLBACK_TIMER_IRQ      IRQn_GPTMR3
#define BOARD_CALLBACK_TIMER_CLK_NAME (clock_gptmr3)

/* APP UART section */
#define BOARD_APP_UART_BASE       HPM_UART3
#define BOARD_APP_UART_IRQ        IRQn_UART3
#define BOARD_APP_UART_BAUDRATE   (115200UL)
#define BOARD_APP_UART_CLK_NAME   clock_uart3

/* APP SPI section */
#define BOARD_APP_SPI_BASE              HPM_SPI1
#define BOARD_APP_SPI_CLK_NAME          clock_spi1
#define BOARD_APP_SPI_IRQ               IRQn_SPI1
#define BOARD_APP_SPI_SCLK_FREQ         (20000000UL)
#define BOARD_APP_SPI_ADDR_LEN_IN_BYTES (1U)
#define BOARD_APP_SPI_DATA_LEN_IN_BITS  (8U)

/* APP I2C section */
#define BOARD_APP_I2C_BASE     HPM_I2C3
#define BOARD_APP_I2C_IRQ      IRQn_I2C3
#define BOARD_APP_I2C_CLK_NAME clock_i2c3

/* APP DMA section */
#define BOARD_APP_XDMA     HPM_XDMA
#define BOARD_APP_XDMA_IRQ IRQn_XDMA
#define BOARD_APP_HDMA     BOARD_APP_XDMA
#define BOARD_APP_HDMA_IRQ BOARD_APP_XDMA_IRQ
#define BOARD_APP_DMAMUX   HPM_DMAMUX

/* APP ADC16 section */
#define BOARD_APP_ADC16_NAME     "ADC0"
#define BOARD_APP_ADC16_BASE     HPM_ADC0
#define BOARD_APP_ADC16_IRQn     IRQn_ADC0
#define BOARD_APP_ADC16_CH_1     (1U)
#define BOARD_APP_ADC16_CLK_NAME (clock_adc0)
#define BOARD_APP_ADC16_CLK_BUS  (clk_adc_src_ahb0)

/* APP MCAN section */
#define BOARD_APP_CAN_BASE HPM_MCAN2
#define BOARD_APP_CAN_IRQn IRQn_MCAN2

/* APP GPTMR section */
#define BOARD_GPTMR          HPM_GPTMR2
#define BOARD_GPTMR_CHANNEL  (0U)
#define BOARD_GPTMR_IRQ      IRQn_GPTMR2
#define BOARD_GPTMR_CLK_NAME clock_gptmr2

/* APP PWMV2 section */
#define BOARD_APP_PWM            HPM_PWM0
#define BOARD_APP_PWM_CLOCK_NAME clock_pwm0
#define BOARD_APP_PWM_OUT1       pwm_channel_0
#define BOARD_APP_PWM_OUT2       pwm_channel_1
#define BOARD_APP_PWM_OUT3       pwm_channel_2
#define BOARD_APP_PWM_OUT4       pwm_channel_3
#define BOARD_APP_PWM_OUT5       pwm_channel_4
#define BOARD_APP_PWM_OUT6       pwm_channel_5
#define BOARD_APP_PWM_FAULT_PIN  (0U)
#define BOARD_APP_PWM_IRQ        IRQn_PWM0

/* APP QEIV2 section */
#define BOARD_BLDC_QEIV2_BASE                    HPM_QEI1
#define BOARD_BLDC_QEIV2_IRQ                     IRQn_QEI1
#define BOARD_BLDC_QEI_MOTOR_PHASE_COUNT_PER_REV (16U)
#define BOARD_BLDC_QEI_CLOCK_SOURCE              clock_qei1
#define BOARD_BLDC_QEI_FOC_PHASE_COUNT_PER_REV   (4000U)

/* ENET section (RTL8211 + RGMII) */
#define BOARD_ENET_MDIO_SHARED_WITH_ESC (1U)
#define BOARD_ENET_RGMII                HPM_ENET0
#define BOARD_ENET_RGMII_TX_DLY         (0U)
#define BOARD_ENET_RGMII_RX_DLY         (0U)
#define BOARD_ENET_RGMII_PTP_CLOCK      (clock_ptp0)
#define BOARD_ENET_RGMII_PPS0_PINOUT    (0U)

#define BOARD_ENET_RGMII_RST_GPIO       HPM_GPIO0
#define BOARD_ENET_RGMII_RST_GPIO_INDEX GPIO_DO_GPIOA
#define BOARD_ENET_RGMII_RST_GPIO_PIN   (21U)

#define BOARD_ENET_PHY_INT_GPIO         HPM_GPIO0
#define BOARD_ENET_PHY_INT_GPIO_INDEX   GPIO_DI_GPIOF
#define BOARD_ENET_PHY_INT_GPIO_PIN     (14U)
#define BOARD_ENET_PHY_INT_IRQ          IRQn_GPIO0_F

#define BOARD_ENET_PHY_ADDR_RTL8211     (2U) /* RTL8211 strap: 010b */

/* EtherCAT section (LAN8720A, ESC port0) */
#define BOARD_ECAT_SUPPORT_PORT1            (0)
#define BOARD_ECAT_SUPPORT_PORT2            (0)
#define BOARD_ECAT_SUPPORT_RUN_ERROR_LED    (0)
#define BOARD_ECAT_RUN_LED_GPIO_AVAILABLE   (0U)
#define BOARD_ECAT_ERROR_LED_GPIO_AVAILABLE (0U)

#define BOARD_ECAT_PORT0_LINK_INVERT        false
#define BOARD_ECAT_PORT1_LINK_INVERT        false
#define BOARD_ECAT_PORT2_LINK_INVERT        false

#define BOARD_ECAT_PHY0_RESET_GPIO          HPM_GPIO0
#define BOARD_ECAT_PHY0_RESET_GPIO_PORT_INDEX GPIO_DO_GPIOA
#define BOARD_ECAT_PHY0_RESET_PIN_INDEX     (9U)
#define BOARD_ECAT_PHY_RESET_LEVEL          (0U)

#define BOARD_ECAT_IN1_GPIO                 HPM_GPIO0
#define BOARD_ECAT_IN1_GPIO_PORT_INDEX      GPIO_DI_GPIOF
#define BOARD_ECAT_IN1_GPIO_PIN_INDEX       (19U)

#define BOARD_ECAT_IN2_GPIO                 HPM_GPIO0
#define BOARD_ECAT_IN2_GPIO_PORT_INDEX      GPIO_DI_GPIOF
#define BOARD_ECAT_IN2_GPIO_PIN_INDEX       (18U)

#define BOARD_ECAT_OUT1_GPIO                HPM_GPIO0
#define BOARD_ECAT_OUT1_GPIO_PORT_INDEX     GPIO_DO_GPIOA
#define BOARD_ECAT_OUT1_GPIO_PIN_INDEX      (3U)

#define BOARD_ECAT_OUT2_GPIO                HPM_GPIO0
#define BOARD_ECAT_OUT2_GPIO_PORT_INDEX     GPIO_DO_GPIOA
#define BOARD_ECAT_OUT2_GPIO_PIN_INDEX      (28U)

#define BOARD_ECAT_OUT3_GPIO                HPM_GPIO0
#define BOARD_ECAT_OUT3_GPIO_PORT_INDEX     GPIO_DO_GPIOA
#define BOARD_ECAT_OUT3_GPIO_PIN_INDEX      (20U)

#define BOARD_ECAT_OUT4_GPIO                HPM_GPIO0
#define BOARD_ECAT_OUT4_GPIO_PORT_INDEX     GPIO_DO_GPIOA
#define BOARD_ECAT_OUT4_GPIO_PIN_INDEX      (19U)

#define BOARD_ECAT_OUT_ON_LEVEL             (1U)

#define BOARD_ECAT_NMII_LINK0_CTRL_INDEX    0

#define BOARD_ECAT_PHY_ADDR_OFFSET          (1U) /* LAN8720 PHYAD0 strapped high */
#define BOARD_ECAT_PORT0_PHY_ADDR           (0U) /* actual addr = offset + port addr = 1 */
#define BOARD_ECAT_PORT1_PHY_ADDR           (0U)
#define BOARD_ECAT_PORT2_PHY_ADDR           (0U)

#define BOARD_ECAT_INIT_EEPROM_I2C          HPM_I2C3
#define BOARD_ECAT_INIT_EEPROM_I2C_CLK      clock_i2c3

/* Reserve flash area for EtherCAT EEPROM emulation */
#define BOARD_ECAT_FLASH_EMULATE_EEPROM_ADDR (0x80000U)

/* XPI NOR parameters used by EtherCAT flash EEPROM emulation helper */
#define BOARD_FLASH_BASE_ADDRESS           (0x80000000UL)
#define BOARD_APP_XPI_NOR_XPI_BASE         (HPM_XPI0)
#define BOARD_APP_XPI_NOR_CFG_OPT_HDR      (0xfcf90002U)
#define BOARD_APP_XPI_NOR_CFG_OPT_OPT0     (0x00000006U)
#define BOARD_APP_XPI_NOR_CFG_OPT_OPT1     (0x00001000U)

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

typedef void (*board_timer_cb)(void);

void board_init(void);
void board_init_core1(void);
void board_init_clock(void);
void board_init_console(void);
void board_print_banner(void);
void board_print_clock_freq(void);
void board_init_gpio_pins(void);
void board_init_led_pins(void);
void board_led_write(uint8_t state);
void board_delay_ms(uint32_t ms);
uint8_t board_get_led_gpio_off_level(void);
void board_timer_create(uint32_t ms, board_timer_cb cb);
uint32_t board_init_uart_clock(UART_Type *ptr);
void board_init_uart(UART_Type *ptr);
uint32_t board_init_i2c_clock(I2C_Type *ptr);
void board_init_i2c(I2C_Type *ptr);
void init_i2c_pins(I2C_Type *ptr);
uint32_t board_init_spi_clock(SPI_Type *ptr);
void board_init_spi_pins(SPI_Type *ptr);
void board_init_spi_pins_with_gpio_as_cs(SPI_Type *ptr);
void board_init_adc16_pins(void);
uint32_t board_init_adc_clock(void *ptr, bool clk_src_bus);
void board_init_can(MCAN_Type *ptr);
uint32_t board_init_can_clock(MCAN_Type *ptr);
uint32_t board_init_gptmr_clock(GPTMR_Type *ptr);
void board_init_gptmr_channel_pin(GPTMR_Type *ptr, uint32_t channel, bool as_output);
void init_gptmr_pins(GPTMR_Type *ptr);
void init_pwm_pins(PWMV2_Type *ptr);
void init_pwm_fault_pins(void);
void init_qeiv2_ab_pins(QEIV2_Type *ptr);
void init_qeiv2_abz_pins(QEIV2_Type *ptr);
void init_qeiv2_uvw_pins(QEIV2_Type *ptr);
void board_init_usb(USB_Type *ptr);

hpm_stat_t board_init_enet_pins(ENET_Type *ptr);
hpm_stat_t board_reset_enet_phy(ENET_Type *ptr);
uint8_t board_get_enet_dma_pbl(ENET_Type *ptr);
hpm_stat_t board_init_enet_rgmii_clock_delay(ENET_Type *ptr);
hpm_stat_t board_init_enet_rmii_reference_clock(ENET_Type *ptr, bool internal);
hpm_stat_t board_init_enet_mii_clock(ENET_Type *ptr);
hpm_stat_t board_init_enet_ptp_clock(ENET_Type *ptr);
hpm_stat_t board_enable_enet_irq(ENET_Type *ptr);
hpm_stat_t board_disable_enet_irq(ENET_Type *ptr);
ENET_Type *board_get_enet_base(uint8_t idx);

void board_select_mdio_owner_for_enet(void);
void board_select_mdio_owner_for_esc(void);
void board_init_ethercat(ESC_Type *ptr);
void board_init_switch_led(void);
void init_esc_eeprom_pin(void);
void init_esc_eeprom_as_i2c_pin(void);

#if defined(__cplusplus)
}
#endif /* __cplusplus */
#endif /* _HPM_BOARD_H */


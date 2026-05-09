/**
 * @file bsp_dma_dispatch_hpm.cpp
 * @brief HPM platform shared DMA interrupt dispatcher
 */

#include "bsp_i2c.h"
#include "bsp_spi.h"
#include "bsp_uart.h"
#include "board.h"

extern "C" {
#include "hpm_interrupt.h"
}

extern "C" void bsp_i2c_dma_irq_handler(bsp_i2c_instance_t i2c);
extern "C" void bsp_spi_dma_irq_handler(bsp_spi_instance_t spi);
extern "C" void bsp_uart_dma_irq_handler(bsp_uart_instance_t uart);

SDK_DECLARE_EXT_ISR_M(BOARD_APP_HDMA_IRQ, hpm_bsp_hdma_isr)
void hpm_bsp_hdma_isr(void)
{
    for (uint32_t spi = 0U; spi < BSP_SPI_MAX; ++spi) {
        bsp_spi_dma_irq_handler(static_cast<bsp_spi_instance_t>(spi));
    }
    for (uint32_t i2c = 0U; i2c < BSP_I2C_MAX; ++i2c) {
        bsp_i2c_dma_irq_handler(static_cast<bsp_i2c_instance_t>(i2c));
    }
    for (uint32_t uart = 0U; uart < BSP_UART_MAX; ++uart) {
        bsp_uart_dma_irq_handler(static_cast<bsp_uart_instance_t>(uart));
    }
}

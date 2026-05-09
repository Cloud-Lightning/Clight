#include "board.h"
#include "bsp_adc16.h"
#include "bsp_dmav2.h"
#include "bsp_gptmr.h"
#include "bsp_i2c.h"
#include "bsp_mcan.h"
#include "bsp_pwmv2.h"
#include "bsp_spi.h"
#include "hpm_interrupt.h"

void bsp_adc16_irq_handler(bsp_adc16_instance_t adc);
void bsp_dmav2_irq_handler(bsp_dmav2_instance_t instance);
void bsp_gptmr_irq_handler(bsp_gptmr_instance_t timer);
void bsp_i2c_irq_handler(bsp_i2c_instance_t i2c);
void bsp_mcan_irq_handler(bsp_mcan_instance_t instance);
void bsp_pwmv2_irq_handler(bsp_pwmv2_instance_t instance);
void bsp_spi_irq_handler(bsp_spi_instance_t spi);
void bsp_spi_dma_irq_handler(bsp_spi_instance_t spi);
void bsp_hpm_gptmr_timer_notify_from_board(void);

SDK_DECLARE_EXT_ISR_M(IRQn_ADC0, hpm_bsp_adc0_isr)
void hpm_bsp_adc0_isr(void)
{
    bsp_adc16_irq_handler(BSP_ADC16_MAIN);
}

SDK_DECLARE_EXT_ISR_M(IRQn_XDMA, hpm_bsp_xdma_isr)
void hpm_bsp_xdma_isr(void)
{
    bsp_dmav2_irq_handler(BSP_DMAV2_MAIN);
}

SDK_DECLARE_EXT_ISR_M(IRQn_HDMA, hpm_bsp_hdma_isr)
void hpm_bsp_hdma_isr(void)
{
    bsp_spi_dma_irq_handler(BSP_SPI_MAIN);
}

SDK_DECLARE_EXT_ISR_M(IRQn_GPTMR2, hpm_bsp_gptmr2_isr)
void hpm_bsp_gptmr2_isr(void)
{
    bsp_gptmr_irq_handler(BSP_GPTMR_CAPTURE);
}

SDK_DECLARE_EXT_ISR_M(IRQn_I2C3, hpm_bsp_i2c3_isr)
void hpm_bsp_i2c3_isr(void)
{
    bsp_i2c_irq_handler(BSP_I2C_MAIN);
}

SDK_DECLARE_EXT_ISR_M(IRQn_MCAN2, hpm_bsp_mcan2_isr)
void hpm_bsp_mcan2_isr(void)
{
    bsp_mcan_irq_handler(BSP_MCAN_MAIN);
}

SDK_DECLARE_EXT_ISR_M(IRQn_PWM0, hpm_bsp_pwm0_isr)
void hpm_bsp_pwm0_isr(void)
{
    bsp_pwmv2_irq_handler(BSP_PWMV2_MAIN);
}

SDK_DECLARE_EXT_ISR_M(IRQn_SPI1, hpm_bsp_spi1_isr)
void hpm_bsp_spi1_isr(void)
{
    bsp_spi_irq_handler(BSP_SPI_MAIN);
}

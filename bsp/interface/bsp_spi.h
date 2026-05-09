/**
 * @file bsp_spi.h
 * @brief BSP SPI interface
 */

#ifndef BSP_SPI_H
#define BSP_SPI_H

#include "bsp_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BSP_SPI_FEATURE_MASTER             (1U << 0)
#define BSP_SPI_FEATURE_BLOCKING           (1U << 1)
#define BSP_SPI_FEATURE_INTERRUPT          (1U << 2)
#define BSP_SPI_FEATURE_DMA_TX             (1U << 3)
#define BSP_SPI_FEATURE_DMA_RX             (1U << 4)
#define BSP_SPI_FEATURE_FULL_DUPLEX        (1U << 5)
#define BSP_SPI_FEATURE_TX_ONLY            (1U << 6)
#define BSP_SPI_FEATURE_RX_ONLY            (1U << 7)
#define BSP_SPI_FEATURE_CONFIGURABLE_CLOCK (1U << 8)
#define BSP_SPI_FEATURE_MSB_FIRST          (1U << 9)
#define BSP_SPI_FEATURE_LSB_FIRST          (1U << 10)

#include "board_config.h"

#ifndef BSP_SPI_LIST
#define BSP_SPI_LIST(X) \
    X(BSP_SPI_MAIN, BSP_SPI_MAIN_BASE, BSP_SPI_MAIN_IRQ, BSP_SPI_MAIN_CLK_NAME, BSP_SPI_MAIN_HAS_PINMUX, \
      BSP_SPI_FEATURE_MASTER | BSP_SPI_FEATURE_BLOCKING | BSP_SPI_FEATURE_MSB_FIRST)
#endif

typedef enum {
#define BSP_SPI_ENUM_ITEM(name, base, irq, clk, has_pinmux, features) name,
    BSP_SPI_LIST(BSP_SPI_ENUM_ITEM)
#undef BSP_SPI_ENUM_ITEM
    BSP_SPI_MAX
} bsp_spi_instance_t;

typedef enum {
    BSP_SPI_MODE_0 = 0,
    BSP_SPI_MODE_1,
    BSP_SPI_MODE_2,
    BSP_SPI_MODE_3,
} bsp_spi_mode_t;

typedef enum {
    BSP_SPI_BIT_ORDER_MSB_FIRST = 0,
    BSP_SPI_BIT_ORDER_LSB_FIRST,
} bsp_spi_bit_order_t;

typedef enum {
    BSP_SPI_EVENT_WRITE = 0,
    BSP_SPI_EVENT_READ,
    BSP_SPI_EVENT_TRANSFER,
} bsp_spi_event_t;

typedef void (*bsp_spi_event_callback_t)(bsp_spi_event_t event, int32_t status, void *arg);

int32_t bsp_spi_init(bsp_spi_instance_t spi,
                     bsp_spi_mode_t mode,
                     uint32_t frequency_hz,
                     bsp_spi_bit_order_t bit_order,
                     bsp_transfer_mode_t transfer_mode,
                     uint8_t irq_priority);
int32_t bsp_spi_transfer(bsp_spi_instance_t spi, const uint8_t *tx_data, uint8_t *rx_data, uint32_t size);
int32_t bsp_spi_transfer_async(bsp_spi_instance_t spi, const uint8_t *tx_data, uint8_t *rx_data, uint32_t size);
int32_t bsp_spi_register_event_callback(bsp_spi_instance_t spi, bsp_spi_event_callback_t callback, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* BSP_SPI_H */

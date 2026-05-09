/**
 * @file bsp_uart.h
 * @brief BSP UART interface
 */

#ifndef BSP_UART_H
#define BSP_UART_H

#include "bsp_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BSP_UART_FEATURE_BLOCKING_TX       (1U << 0)
#define BSP_UART_FEATURE_INTERRUPT_TX      (1U << 1)
#define BSP_UART_FEATURE_INTERRUPT_RX      (1U << 2)
#define BSP_UART_FEATURE_DMA_TX            (1U << 3)
#define BSP_UART_FEATURE_DMA_RX            (1U << 4)
#define BSP_UART_FEATURE_RX_IDLE           (1U << 5)
#define BSP_UART_FEATURE_CONFIGURABLE_BAUD (1U << 6)
#define BSP_UART_FEATURE_PARITY_NONE       (1U << 7)
#define BSP_UART_FEATURE_PARITY_ODD        (1U << 8)
#define BSP_UART_FEATURE_PARITY_EVEN       (1U << 9)
#define BSP_UART_FEATURE_PARITY_FORCED     (1U << 10)
#define BSP_UART_FEATURE_STOP_1            (1U << 11)
#define BSP_UART_FEATURE_STOP_1_5          (1U << 12)
#define BSP_UART_FEATURE_STOP_2            (1U << 13)
#define BSP_UART_FEATURE_WORD_7            (1U << 14)
#define BSP_UART_FEATURE_WORD_8            (1U << 15)

#include "board_config.h"

/*
 * UART resource list comes from board_config.h.
 * Fallback keeps compatibility with legacy board_config definitions.
 */
#ifndef BSP_UART_LIST
#define BSP_UART_LIST(X) \
    X(BSP_UART_DEBUG, BSP_UART_DEBUG_BASE, BSP_UART_DEBUG_IRQ, \
      BSP_UART_FEATURE_BLOCKING_TX | BSP_UART_FEATURE_CONFIGURABLE_BAUD | \
      BSP_UART_FEATURE_PARITY_NONE | BSP_UART_FEATURE_STOP_1 | BSP_UART_FEATURE_WORD_8)
#endif

typedef enum {
#define BSP_UART_ENUM_ITEM(name, base, irq, features) name,
    BSP_UART_LIST(BSP_UART_ENUM_ITEM)
#undef BSP_UART_ENUM_ITEM
    BSP_UART_MAX
} bsp_uart_instance_t;

typedef enum {
    BSP_UART_PARITY_NONE = 0,
    BSP_UART_PARITY_ODD,
    BSP_UART_PARITY_EVEN,
    BSP_UART_PARITY_ALWAYS_1,
    BSP_UART_PARITY_ALWAYS_0,
} bsp_uart_parity_t;

typedef enum {
    BSP_UART_STOP_BITS_1 = 0,
    BSP_UART_STOP_BITS_1_5,
    BSP_UART_STOP_BITS_2,
} bsp_uart_stop_bits_t;

typedef enum {
    BSP_UART_WORD_LENGTH_5 = 0,
    BSP_UART_WORD_LENGTH_6,
    BSP_UART_WORD_LENGTH_7,
    BSP_UART_WORD_LENGTH_8,
} bsp_uart_word_length_t;

typedef enum {
    BSP_UART_EVENT_RX_READY = 0,
    BSP_UART_EVENT_RX_DONE,
    BSP_UART_EVENT_RX_HALF,
    BSP_UART_EVENT_RX_IDLE,
    BSP_UART_EVENT_RX_TIMEOUT,
    BSP_UART_EVENT_TX_DONE,
    BSP_UART_EVENT_TX_HALF,
    BSP_UART_EVENT_ERROR,
} bsp_uart_event_t;

typedef void (*bsp_uart_event_callback_t)(bsp_uart_event_t event, uint8_t *data, uint16_t len, int32_t status, void *arg);

int32_t bsp_uart_init(bsp_uart_instance_t uart,
                      uint32_t baudrate,
                      bsp_uart_parity_t parity,
                      bsp_uart_stop_bits_t stop_bits,
                      bsp_uart_word_length_t word_length,
                      bsp_transfer_mode_t transfer_mode,
                      uint8_t irq_priority);
int32_t bsp_uart_send(bsp_uart_instance_t uart, uint8_t *data, uint16_t len, uint32_t timeout);
int32_t bsp_uart_send_async(bsp_uart_instance_t uart, const uint8_t *data, uint16_t len);
int32_t bsp_uart_receive_it(bsp_uart_instance_t uart, uint8_t *rx_buf, uint16_t max_len);
int32_t bsp_uart_register_event_callback(bsp_uart_instance_t uart, bsp_uart_event_callback_t callback, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* BSP_UART_H */

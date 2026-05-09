#include "bsp_uart.h"

#include "freertos/FreeRTOS.h"
#include "driver/uart.h"

typedef struct {
    uart_port_t port;
    int tx_pin;
    int rx_pin;
    bool initialized;
    uint8_t *rx_buffer;
    uint16_t rx_length;
    bsp_uart_event_callback_t event_callback;
    void *event_callback_arg;
} uart_instance_map_t;

#define BSP_UART_MAP_ITEM(name, base, irq, features) \
    { static_cast<uart_port_t>(base), static_cast<int>(name##_TX_PIN), static_cast<int>(name##_RX_PIN), false, nullptr, 0U, nullptr, nullptr },
static uart_instance_map_t s_uart_map[BSP_UART_MAX] = {
    BSP_UART_LIST(BSP_UART_MAP_ITEM)
};
#undef BSP_UART_MAP_ITEM

static void emit_uart_event(uart_instance_map_t *map,
                            bsp_uart_event_t event,
                            uint8_t *data,
                            uint16_t len,
                            int32_t status)
{
    if (map->event_callback != nullptr) {
        map->event_callback(event, data, len, status, map->event_callback_arg);
    }
}

int32_t bsp_uart_init(bsp_uart_instance_t uart,
                      uint32_t baudrate,
                      bsp_uart_parity_t parity,
                      bsp_uart_stop_bits_t stop_bits,
                      bsp_uart_word_length_t word_length,
                      bsp_transfer_mode_t transfer_mode,
                      uint8_t irq_priority)
{
    (void) transfer_mode;
    (void) irq_priority;
    if (uart >= BSP_UART_MAX) {
        return BSP_ERROR_PARAM;
    }

    auto *map = &s_uart_map[uart];
    if (map->initialized) {
        return BSP_OK;
    }

    uart_parity_t uart_parity = UART_PARITY_DISABLE;
    if (parity == BSP_UART_PARITY_ODD) {
        uart_parity = UART_PARITY_ODD;
    } else if (parity == BSP_UART_PARITY_EVEN) {
        uart_parity = UART_PARITY_EVEN;
    } else if (parity != BSP_UART_PARITY_NONE) {
        return BSP_ERROR_UNSUPPORTED;
    }

    uart_word_length_t uart_word_length = UART_DATA_8_BITS;
    switch (word_length) {
    case BSP_UART_WORD_LENGTH_5:
        uart_word_length = UART_DATA_5_BITS;
        break;
    case BSP_UART_WORD_LENGTH_6:
        uart_word_length = UART_DATA_6_BITS;
        break;
    case BSP_UART_WORD_LENGTH_7:
        uart_word_length = UART_DATA_7_BITS;
        break;
    case BSP_UART_WORD_LENGTH_8:
        uart_word_length = UART_DATA_8_BITS;
        break;
    default:
        return BSP_ERROR_UNSUPPORTED;
    }

    uart_stop_bits_t uart_stop_bits = UART_STOP_BITS_1;
    switch (stop_bits) {
    case BSP_UART_STOP_BITS_1_5:
        uart_stop_bits = UART_STOP_BITS_1_5;
        break;
    case BSP_UART_STOP_BITS_2:
        uart_stop_bits = UART_STOP_BITS_2;
        break;
    case BSP_UART_STOP_BITS_1:
        uart_stop_bits = UART_STOP_BITS_1;
        break;
    default:
        return BSP_ERROR_UNSUPPORTED;
    }

    const uart_config_t config = {
        .baud_rate = static_cast<int>((baudrate != 0U) ? baudrate : BSP_UART_DEFAULT_BAUDRATE),
        .data_bits = uart_word_length,
        .parity = uart_parity,
        .stop_bits = uart_stop_bits,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT,
    };

    if (uart_param_config(map->port, &config) != ESP_OK) {
        return BSP_ERROR;
    }
    if (uart_set_pin(map->port, map->tx_pin, map->rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK) {
        return BSP_ERROR;
    }
    const auto install_status = uart_driver_install(map->port, 1024, 0, 0, nullptr, 0);
    if (install_status != ESP_OK && install_status != ESP_ERR_INVALID_STATE) {
        return BSP_ERROR;
    }

    map->initialized = true;
    return BSP_OK;
}

int32_t bsp_uart_send(bsp_uart_instance_t uart, uint8_t *data, uint16_t len, uint32_t timeout)
{
    if ((uart >= BSP_UART_MAX) || (data == nullptr) || (len == 0U)) {
        return BSP_ERROR_PARAM;
    }
    if (bsp_uart_init(uart, 0U, BSP_UART_PARITY_NONE, BSP_UART_STOP_BITS_1, BSP_UART_WORD_LENGTH_8, BSP_TRANSFER_MODE_AUTO, 0U) != BSP_OK) {
        return BSP_ERROR;
    }

    const auto written = uart_write_bytes(s_uart_map[uart].port, data, len);
    if (written < 0) {
        return BSP_ERROR_TIMEOUT;
    }
    if (timeout != 0U) {
        return (uart_wait_tx_done(s_uart_map[uart].port, pdMS_TO_TICKS(timeout)) == ESP_OK) ? BSP_OK : BSP_ERROR_TIMEOUT;
    }
    return BSP_OK;
}

int32_t bsp_uart_send_async(bsp_uart_instance_t uart, const uint8_t *data, uint16_t len)
{
    if ((uart >= BSP_UART_MAX) || (data == nullptr) || (len == 0U)) {
        return BSP_ERROR_PARAM;
    }
    if (bsp_uart_init(uart, 0U, BSP_UART_PARITY_NONE, BSP_UART_STOP_BITS_1, BSP_UART_WORD_LENGTH_8, BSP_TRANSFER_MODE_AUTO, 0U) != BSP_OK) {
        return BSP_ERROR;
    }

    auto &map = s_uart_map[uart];
    const auto written = uart_write_bytes(map.port, data, len);
    if (written < 0) {
        emit_uart_event(&map, BSP_UART_EVENT_ERROR, nullptr, 0U, BSP_ERROR);
        return BSP_ERROR;
    }

    emit_uart_event(&map, BSP_UART_EVENT_TX_DONE, nullptr, 0U, BSP_OK);
    return BSP_OK;
}

int32_t bsp_uart_receive_it(bsp_uart_instance_t uart, uint8_t *rx_buf, uint16_t max_len)
{
    if ((uart >= BSP_UART_MAX) || (rx_buf == nullptr) || (max_len == 0U)) {
        return BSP_ERROR_PARAM;
    }
    if (bsp_uart_init(uart, 0U, BSP_UART_PARITY_NONE, BSP_UART_STOP_BITS_1, BSP_UART_WORD_LENGTH_8, BSP_TRANSFER_MODE_AUTO, 0U) != BSP_OK) {
        return BSP_ERROR;
    }

    auto &map = s_uart_map[uart];
    map.rx_buffer = rx_buf;
    map.rx_length = max_len;

    const auto read = uart_read_bytes(map.port, rx_buf, max_len, 0);
    if (read <= 0) {
        emit_uart_event(&map, BSP_UART_EVENT_RX_TIMEOUT, rx_buf, 0U, BSP_ERROR_TIMEOUT);
        return BSP_ERROR_TIMEOUT;
    }
    emit_uart_event(&map, BSP_UART_EVENT_RX_DONE, rx_buf, static_cast<uint16_t>(read), BSP_OK);
    return BSP_OK;
}

int32_t bsp_uart_register_event_callback(bsp_uart_instance_t uart, bsp_uart_event_callback_t callback, void *arg)
{
    if (uart >= BSP_UART_MAX) {
        return BSP_ERROR_PARAM;
    }

    s_uart_map[uart].event_callback = callback;
    s_uart_map[uart].event_callback_arg = arg;
    return BSP_OK;
}

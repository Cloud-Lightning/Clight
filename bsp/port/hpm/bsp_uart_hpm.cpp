/**
 * @file bsp_uart_hpm.cpp
 * @brief HPM platform implementation for BSP UART interface
 */

#include "bsp_uart.h"
#include "board_config.h"
#include "board.h"

extern "C" {
#include "hpm_dmav2_drv.h"
#include "hpm_dmamux_drv.h"
#include "hpm_uart_drv.h"
#include "hpm_interrupt.h"
}

typedef struct {
    UART_Type *base;
    uint32_t irq;
    uint32_t features;
    bool initialized;
    bsp_transfer_mode_t transfer_mode;
    uint8_t irq_priority;
    uint8_t *rx_buf;
    uint16_t rx_max_len;
    bool rx_dma_active;
    bool tx_dma_active;
    bsp_uart_event_callback_t event_callback;
    void *event_callback_arg;
} uart_instance_map_t;

#define BSP_UART_MAP_ITEM(name, base_addr, irq_num, features) \
    { base_addr, irq_num, static_cast<uint32_t>(features), false, BSP_TRANSFER_MODE_AUTO, BSP_IRQ_PRIORITY_DEFAULT, nullptr, 0U, false, false, nullptr, nullptr },
static uart_instance_map_t uart_map[BSP_UART_MAX] = {
    BSP_UART_LIST(BSP_UART_MAP_ITEM)
};
#undef BSP_UART_MAP_ITEM

typedef struct {
    DMA_Type *base;
    uint32_t irq;
    DMAMUX_Type *dmamux;
    uint32_t rx_req;
    uint32_t tx_req;
    uint8_t rx_ch;
    uint8_t tx_ch;
} uart_dma_resource_t;

#define BSP_UART_DMA_MAP_ITEM(name, base_addr, irq_num, features) \
    { name##_DMA_CONTROLLER, name##_DMA_IRQ, name##_DMAMUX, name##_DMA_RX_REQ, name##_DMA_TX_REQ, name##_DMA_RX_CH, name##_DMA_TX_CH },
static const uart_dma_resource_t s_uart_dma_map[BSP_UART_MAX] = {
    BSP_UART_LIST(BSP_UART_DMA_MAP_ITEM)
};
#undef BSP_UART_DMA_MAP_ITEM

static uint8_t normalize_irq_priority(uint8_t irq_priority)
{
    return (irq_priority == 0U) ? BSP_IRQ_PRIORITY_DEFAULT : irq_priority;
}

static bool uart_has_feature(const uart_instance_map_t &map, uint32_t feature)
{
    return (map.features & feature) != 0U;
}

static bool uart_dma_resource_present(const uart_dma_resource_t &dma)
{
    return (dma.base != nullptr) && (dma.dmamux != nullptr);
}

static bool uart_tx_dma_available(const uart_instance_map_t &map, const uart_dma_resource_t &dma)
{
    return uart_has_feature(map, BSP_UART_FEATURE_DMA_TX) && uart_dma_resource_present(dma) && (dma.tx_req != UINT32_MAX);
}

static bool uart_rx_dma_available(const uart_instance_map_t &map, const uart_dma_resource_t &dma)
{
    return uart_has_feature(map, BSP_UART_FEATURE_DMA_RX) && uart_dma_resource_present(dma) && (dma.rx_req != UINT32_MAX);
}

static bool uart_parity_supported(const uart_instance_map_t &map, bsp_uart_parity_t parity)
{
    switch (parity) {
    case BSP_UART_PARITY_NONE:
        return uart_has_feature(map, BSP_UART_FEATURE_PARITY_NONE);
    case BSP_UART_PARITY_ODD:
        return uart_has_feature(map, BSP_UART_FEATURE_PARITY_ODD);
    case BSP_UART_PARITY_EVEN:
        return uart_has_feature(map, BSP_UART_FEATURE_PARITY_EVEN);
    case BSP_UART_PARITY_ALWAYS_1:
    case BSP_UART_PARITY_ALWAYS_0:
        return uart_has_feature(map, BSP_UART_FEATURE_PARITY_FORCED);
    default:
        return false;
    }
}

static bool uart_stop_bits_supported(const uart_instance_map_t &map, bsp_uart_stop_bits_t stop_bits)
{
    switch (stop_bits) {
    case BSP_UART_STOP_BITS_1:
        return uart_has_feature(map, BSP_UART_FEATURE_STOP_1);
    case BSP_UART_STOP_BITS_1_5:
        return uart_has_feature(map, BSP_UART_FEATURE_STOP_1_5);
    case BSP_UART_STOP_BITS_2:
        return uart_has_feature(map, BSP_UART_FEATURE_STOP_2);
    default:
        return false;
    }
}

static bool uart_word_length_supported(const uart_instance_map_t &map, bsp_uart_word_length_t word_length)
{
    switch (word_length) {
    case BSP_UART_WORD_LENGTH_7:
        return uart_has_feature(map, BSP_UART_FEATURE_WORD_7);
    case BSP_UART_WORD_LENGTH_8:
        return uart_has_feature(map, BSP_UART_FEATURE_WORD_8);
    default:
        return false;
    }
}

static int32_t validate_uart_init_mode(const uart_instance_map_t &map,
                                       const uart_dma_resource_t &dma,
                                       bsp_transfer_mode_t transfer_mode)
{
    switch (transfer_mode) {
    case BSP_TRANSFER_MODE_AUTO:
        return BSP_OK;
    case BSP_TRANSFER_MODE_BLOCKING:
        return uart_has_feature(map, BSP_UART_FEATURE_BLOCKING_TX) ? BSP_OK : BSP_ERROR_UNSUPPORTED;
    case BSP_TRANSFER_MODE_INTERRUPT:
        return (uart_has_feature(map, BSP_UART_FEATURE_INTERRUPT_TX) || uart_has_feature(map, BSP_UART_FEATURE_INTERRUPT_RX))
                   ? BSP_OK
                   : BSP_ERROR_UNSUPPORTED;
    case BSP_TRANSFER_MODE_DMA:
        return (uart_tx_dma_available(map, dma) || uart_rx_dma_available(map, dma))
                   ? BSP_OK
                   : BSP_ERROR_UNSUPPORTED;
    default:
        return BSP_ERROR_PARAM;
    }
}

static bool uart_init_uses_dma(const uart_instance_map_t &map,
                               const uart_dma_resource_t &dma,
                               bsp_transfer_mode_t transfer_mode)
{
    if (transfer_mode == BSP_TRANSFER_MODE_DMA) {
        return true;
    }
    return (transfer_mode == BSP_TRANSFER_MODE_AUTO) && (uart_tx_dma_available(map, dma) || uart_rx_dma_available(map, dma));
}

static int32_t resolve_uart_tx_mode(const uart_instance_map_t &map,
                                    const uart_dma_resource_t &dma,
                                    bsp_transfer_mode_t *out_mode)
{
    switch (map.transfer_mode) {
    case BSP_TRANSFER_MODE_AUTO:
        if (uart_tx_dma_available(map, dma)) {
            *out_mode = BSP_TRANSFER_MODE_DMA;
            return BSP_OK;
        }
        if (uart_has_feature(map, BSP_UART_FEATURE_INTERRUPT_TX)) {
            *out_mode = BSP_TRANSFER_MODE_INTERRUPT;
            return BSP_OK;
        }
        return BSP_ERROR_UNSUPPORTED;
    case BSP_TRANSFER_MODE_DMA:
        if (uart_tx_dma_available(map, dma)) {
            *out_mode = BSP_TRANSFER_MODE_DMA;
            return BSP_OK;
        }
        return BSP_ERROR_UNSUPPORTED;
    case BSP_TRANSFER_MODE_INTERRUPT:
        if (uart_has_feature(map, BSP_UART_FEATURE_INTERRUPT_TX)) {
            *out_mode = BSP_TRANSFER_MODE_INTERRUPT;
            return BSP_OK;
        }
        return BSP_ERROR_UNSUPPORTED;
    default:
        return BSP_ERROR_UNSUPPORTED;
    }
}

static int32_t resolve_uart_rx_mode(const uart_instance_map_t &map,
                                    const uart_dma_resource_t &dma,
                                    bsp_transfer_mode_t *out_mode)
{
    switch (map.transfer_mode) {
    case BSP_TRANSFER_MODE_AUTO:
        if (uart_rx_dma_available(map, dma)) {
            *out_mode = BSP_TRANSFER_MODE_DMA;
            return BSP_OK;
        }
        if (uart_has_feature(map, BSP_UART_FEATURE_INTERRUPT_RX)) {
            *out_mode = BSP_TRANSFER_MODE_INTERRUPT;
            return BSP_OK;
        }
        return BSP_ERROR_UNSUPPORTED;
    case BSP_TRANSFER_MODE_DMA:
        if (uart_rx_dma_available(map, dma)) {
            *out_mode = BSP_TRANSFER_MODE_DMA;
            return BSP_OK;
        }
        return BSP_ERROR_UNSUPPORTED;
    case BSP_TRANSFER_MODE_INTERRUPT:
        if (uart_has_feature(map, BSP_UART_FEATURE_INTERRUPT_RX)) {
            *out_mode = BSP_TRANSFER_MODE_INTERRUPT;
            return BSP_OK;
        }
        return BSP_ERROR_UNSUPPORTED;
    default:
        return BSP_ERROR_UNSUPPORTED;
    }
}

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

static uint32_t default_uart_baudrate(UART_Type *base)
{
    return (base == BOARD_APP_UART_BASE) ? BOARD_APP_UART_BAUDRATE : BOARD_CONSOLE_UART_BAUDRATE;
}

static int32_t setup_uart_dma_channel(const uart_dma_resource_t &dma,
                                      uint8_t channel,
                                      uint32_t source,
                                      uint32_t destination,
                                      bool source_fixed,
                                      bool destination_fixed,
                                      uint32_t size)
{
    dma_handshake_config_t config;
    dma_default_handshake_config(dma.base, &config);
    config.ch_index = channel;
    config.src = source;
    config.dst = destination;
    config.src_fixed = source_fixed;
    config.dst_fixed = destination_fixed;
    config.data_width = DMA_TRANSFER_WIDTH_BYTE;
    config.size_in_byte = size;
    return (dma_setup_handshake(dma.base, &config, false) == status_success) ? BSP_OK : BSP_ERROR;
}

static void stop_uart_dma_channel(const uart_dma_resource_t &dma, uint8_t channel)
{
    dma_disable_channel_interrupt(dma.base, channel, DMA_INTERRUPT_MASK_ALL);
    dma_disable_channel(dma.base, channel);
    dma_clear_transfer_status(dma.base, channel);
}

static uint8_t to_hpm_uart_parity(bsp_uart_parity_t parity)
{
    switch (parity) {
    case BSP_UART_PARITY_ODD:
        return parity_odd;
    case BSP_UART_PARITY_EVEN:
        return parity_even;
    case BSP_UART_PARITY_ALWAYS_1:
        return parity_always_1;
    case BSP_UART_PARITY_ALWAYS_0:
        return parity_always_0;
    case BSP_UART_PARITY_NONE:
    default:
        return parity_none;
    }
}

static uint8_t to_hpm_uart_stop_bits(bsp_uart_stop_bits_t stop_bits)
{
    switch (stop_bits) {
    case BSP_UART_STOP_BITS_1_5:
        return stop_bits_1_5;
    case BSP_UART_STOP_BITS_2:
        return stop_bits_2;
    case BSP_UART_STOP_BITS_1:
    default:
        return stop_bits_1;
    }
}

static uint8_t to_hpm_uart_word_length(bsp_uart_word_length_t word_length)
{
    switch (word_length) {
    case BSP_UART_WORD_LENGTH_5:
        return word_length_5_bits;
    case BSP_UART_WORD_LENGTH_6:
        return word_length_6_bits;
    case BSP_UART_WORD_LENGTH_7:
        return word_length_7_bits;
    case BSP_UART_WORD_LENGTH_8:
    default:
        return word_length_8_bits;
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
    if (uart >= BSP_UART_MAX) {
        return BSP_ERROR_PARAM;
    }

    uart_instance_map_t *map = &uart_map[uart];
    const auto &dma = s_uart_dma_map[uart];
    uart_config_t config;
    uint32_t src_freq;

    if ((baudrate != 0U) && !uart_has_feature(*map, BSP_UART_FEATURE_CONFIGURABLE_BAUD)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (!uart_parity_supported(*map, parity) || !uart_stop_bits_supported(*map, stop_bits) || !uart_word_length_supported(*map, word_length)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    const auto mode_status = validate_uart_init_mode(*map, dma, transfer_mode);
    if (mode_status != BSP_OK) {
        return mode_status;
    }

    board_init_uart(map->base);
    src_freq = board_init_uart_clock(map->base);
    if (src_freq == 0U) {
        return BSP_ERROR_UNSUPPORTED;
    }

    uart_default_config(map->base, &config);
    config.src_freq_in_hz = src_freq;
    config.baudrate = (baudrate != 0U) ? baudrate : default_uart_baudrate(map->base);
    config.parity = to_hpm_uart_parity(parity);
    config.num_of_stop_bits = to_hpm_uart_stop_bits(stop_bits);
    config.word_length = to_hpm_uart_word_length(word_length);
    config.dma_enable = uart_init_uses_dma(*map, dma, transfer_mode);
#if defined(HPM_IP_FEATURE_UART_RX_IDLE_DETECT) && (HPM_IP_FEATURE_UART_RX_IDLE_DETECT == 1)
    config.rxidle_config.detect_enable = uart_has_feature(*map, BSP_UART_FEATURE_RX_IDLE);
    config.rxidle_config.detect_irq_enable = uart_has_feature(*map, BSP_UART_FEATURE_RX_IDLE);
    config.rxidle_config.idle_cond = uart_rxline_idle_cond_rxline_logic_one;
    config.rxidle_config.threshold = 10U;
#endif
    if (uart_init(map->base, &config) != status_success) {
        return BSP_ERROR;
    }

    map->transfer_mode = transfer_mode;
    map->irq_priority = normalize_irq_priority(irq_priority);
    map->rx_dma_active = false;
    map->tx_dma_active = false;
    map->initialized = true;
    return BSP_OK;
}

int32_t bsp_uart_send(bsp_uart_instance_t uart, uint8_t *data, uint16_t len, uint32_t timeout)
{
    (void) timeout;

    if ((uart >= BSP_UART_MAX) || (data == nullptr) || (len == 0U)) {
        return BSP_ERROR_PARAM;
    }

    uart_instance_map_t *map = &uart_map[uart];
    if (!map->initialized) {
        const auto status = bsp_uart_init(uart,
                                          0U,
                                          BSP_UART_PARITY_NONE,
                                          BSP_UART_STOP_BITS_1,
                                          BSP_UART_WORD_LENGTH_8,
                                          BSP_TRANSFER_MODE_AUTO,
                                          0U);
        if (status != BSP_OK) {
            return status;
        }
    }
    if (!uart_has_feature(*map, BSP_UART_FEATURE_BLOCKING_TX)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    for (uint16_t i = 0U; i < len; i++) {
        uart_send_byte(map->base, data[i]);
    }

    return BSP_OK;
}

int32_t bsp_uart_send_async(bsp_uart_instance_t uart, const uint8_t *data, uint16_t len)
{
    if ((uart >= BSP_UART_MAX) || (data == nullptr) || (len == 0U)) {
        return BSP_ERROR_PARAM;
    }

    uart_instance_map_t *map = &uart_map[uart];
    const auto &dma = s_uart_dma_map[uart];
    if (!map->initialized) {
        const auto status = bsp_uart_init(uart,
                                          0U,
                                          BSP_UART_PARITY_NONE,
                                          BSP_UART_STOP_BITS_1,
                                          BSP_UART_WORD_LENGTH_8,
                                          BSP_TRANSFER_MODE_AUTO,
                                          0U);
        if (status != BSP_OK) {
            return status;
        }
    }
    bsp_transfer_mode_t tx_mode = BSP_TRANSFER_MODE_AUTO;
    const auto mode_status = resolve_uart_tx_mode(*map, dma, &tx_mode);
    if (mode_status != BSP_OK) {
        return mode_status;
    }
    if (tx_mode != BSP_TRANSFER_MODE_DMA) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (map->tx_dma_active) {
        return BSP_ERROR_BUSY;
    }

    stop_uart_dma_channel(dma, dma.tx_ch);
    dmamux_config(dma.dmamux,
                  DMA_SOC_CHN_TO_DMAMUX_CHN(dma.base, dma.tx_ch),
                  dma.tx_req,
                  true);
    if (setup_uart_dma_channel(dma,
                               dma.tx_ch,
                               core_local_mem_to_sys_address(BOARD_RUNNING_CORE,
                                                             static_cast<uint32_t>(reinterpret_cast<uintptr_t>(data))),
                               static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&map->base->THR)),
                               false,
                               true,
                               len) != BSP_OK) {
        stop_uart_dma_channel(dma, dma.tx_ch);
        return BSP_ERROR;
    }
    dma_enable_channel_interrupt(dma.base,
                                 dma.tx_ch,
                                 DMA_INTERRUPT_MASK_TERMINAL_COUNT | DMA_INTERRUPT_MASK_HALF_TC | DMA_INTERRUPT_MASK_ERROR | DMA_INTERRUPT_MASK_ABORT);
    intc_m_enable_irq_with_priority(dma.irq, map->irq_priority);
    map->tx_dma_active = true;
    if (dma_enable_channel(dma.base, dma.tx_ch) != status_success) {
        stop_uart_dma_channel(dma, dma.tx_ch);
        map->tx_dma_active = false;
        return BSP_ERROR;
    }
    return BSP_OK;
}

int32_t bsp_uart_receive_it(bsp_uart_instance_t uart, uint8_t *rx_buf, uint16_t max_len)
{
    if ((uart >= BSP_UART_MAX) || (rx_buf == nullptr) || (max_len == 0U)) {
        return BSP_ERROR_PARAM;
    }

    uart_instance_map_t *map = &uart_map[uart];
    const auto &dma = s_uart_dma_map[uart];
    if (!map->initialized) {
        const auto status = bsp_uart_init(uart,
                                          0U,
                                          BSP_UART_PARITY_NONE,
                                          BSP_UART_STOP_BITS_1,
                                          BSP_UART_WORD_LENGTH_8,
                                          BSP_TRANSFER_MODE_AUTO,
                                          0U);
        if (status != BSP_OK) {
            return status;
        }
    }
    bsp_transfer_mode_t mode = BSP_TRANSFER_MODE_AUTO;
    const auto mode_status = resolve_uart_rx_mode(*map, dma, &mode);
    if (mode_status != BSP_OK) {
        return mode_status;
    }
    map->rx_buf = rx_buf;
    map->rx_max_len = max_len;

    if (mode == BSP_TRANSFER_MODE_DMA) {
        if (map->rx_dma_active) {
            return BSP_ERROR_BUSY;
        }

        stop_uart_dma_channel(dma, dma.rx_ch);
        dmamux_config(dma.dmamux,
                      DMA_SOC_CHN_TO_DMAMUX_CHN(dma.base, dma.rx_ch),
                      dma.rx_req,
                      true);
        if (setup_uart_dma_channel(dma,
                                   dma.rx_ch,
                                   static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&map->base->RBR)),
                                   core_local_mem_to_sys_address(BOARD_RUNNING_CORE,
                                                                 static_cast<uint32_t>(reinterpret_cast<uintptr_t>(rx_buf))),
                                   true,
                                   false,
                                   max_len) != BSP_OK) {
            stop_uart_dma_channel(dma, dma.rx_ch);
            return BSP_ERROR;
        }
        dma_enable_channel_interrupt(dma.base,
                                     dma.rx_ch,
                                     DMA_INTERRUPT_MASK_TERMINAL_COUNT | DMA_INTERRUPT_MASK_HALF_TC | DMA_INTERRUPT_MASK_ERROR | DMA_INTERRUPT_MASK_ABORT);
        intc_m_enable_irq_with_priority(dma.irq, map->irq_priority);
        intc_m_enable_irq_with_priority(map->irq, map->irq_priority);
        map->rx_dma_active = true;
        if (dma_enable_channel(dma.base, dma.rx_ch) != status_success) {
            stop_uart_dma_channel(dma, dma.rx_ch);
            map->rx_dma_active = false;
            return BSP_ERROR;
        }
        return BSP_OK;
    }

    uart_enable_irq(map->base, uart_intr_rx_data_avail_or_timeout);
    intc_m_enable_irq_with_priority(map->irq, map->irq_priority);
    return BSP_OK;
}

int32_t bsp_uart_register_event_callback(bsp_uart_instance_t uart, bsp_uart_event_callback_t callback, void *arg)
{
    if (uart >= BSP_UART_MAX) {
        return BSP_ERROR_PARAM;
    }

    uart_instance_map_t *map = &uart_map[uart];
    map->event_callback = callback;
    map->event_callback_arg = arg;
    return BSP_OK;
}

void bsp_uart_irq_handler(bsp_uart_instance_t uart)
{
    if (uart >= BSP_UART_MAX) {
        return;
    }

    uart_instance_map_t *map = &uart_map[uart];
    const uint8_t irq_id = uart_get_irq_id(map->base);

    if (uart_is_rxline_idle(map->base)) {
        uart_clear_rxline_idle_flag(map->base);
        if (map->rx_dma_active) {
            const auto &dma = s_uart_dma_map[uart];
            const uint16_t received = static_cast<uint16_t>(map->rx_max_len - dma_get_remaining_transfer_size(dma.base, dma.rx_ch));
            stop_uart_dma_channel(dma, dma.rx_ch);
            map->rx_dma_active = false;
            emit_uart_event(map, BSP_UART_EVENT_RX_IDLE, map->rx_buf, received, BSP_OK);
        } else {
            emit_uart_event(map, BSP_UART_EVENT_RX_IDLE, map->rx_buf, 0U, BSP_OK);
        }
    }

    if (!map->rx_dma_active && (irq_id == uart_intr_id_rx_timeout)) {
        emit_uart_event(map, BSP_UART_EVENT_RX_TIMEOUT, map->rx_buf, 0U, BSP_OK);
    }

    if (uart_check_status(map->base, uart_stat_overrun_error)
        || uart_check_status(map->base, uart_stat_parity_error)
        || uart_check_status(map->base, uart_stat_framing_error)
        || uart_check_status(map->base, uart_stat_rx_fifo_error)) {
        emit_uart_event(map, BSP_UART_EVENT_ERROR, map->rx_buf, 0U, BSP_ERROR);
    }

    if (!map->rx_dma_active && uart_check_status(map->base, uart_stat_data_ready)) {
        uint8_t data = 0U;
        uart_receive_byte(map->base, &data);
        if (map->rx_buf != nullptr) {
            map->rx_buf[0] = data;
            emit_uart_event(map, BSP_UART_EVENT_RX_READY, map->rx_buf, 1U, BSP_OK);
        }
    }
}

extern "C" void bsp_uart_dma_irq_handler(bsp_uart_instance_t uart)
{
    if (uart >= BSP_UART_MAX) {
        return;
    }

    uart_instance_map_t *map = &uart_map[uart];
    const auto &dma = s_uart_dma_map[uart];

    if (map->rx_dma_active) {
        const uint32_t status = dma_check_transfer_status(dma.base, dma.rx_ch);
        if ((status & (DMA_CHANNEL_STATUS_ERROR | DMA_CHANNEL_STATUS_ABORT)) != 0U) {
            stop_uart_dma_channel(dma, dma.rx_ch);
            map->rx_dma_active = false;
            emit_uart_event(map, BSP_UART_EVENT_ERROR, map->rx_buf, 0U, BSP_ERROR);
        }
        if ((status & DMA_CHANNEL_STATUS_HALF_TC) != 0U) {
            const uint16_t received = static_cast<uint16_t>(map->rx_max_len / 2U);
            emit_uart_event(map, BSP_UART_EVENT_RX_HALF, map->rx_buf, received, BSP_OK);
        }
        if ((status & DMA_CHANNEL_STATUS_TC) != 0U) {
            stop_uart_dma_channel(dma, dma.rx_ch);
            map->rx_dma_active = false;
            emit_uart_event(map, BSP_UART_EVENT_RX_DONE, map->rx_buf, map->rx_max_len, BSP_OK);
        }
    }

    if (map->tx_dma_active) {
        const uint32_t status = dma_check_transfer_status(dma.base, dma.tx_ch);
        if ((status & (DMA_CHANNEL_STATUS_ERROR | DMA_CHANNEL_STATUS_ABORT)) != 0U) {
            stop_uart_dma_channel(dma, dma.tx_ch);
            map->tx_dma_active = false;
            emit_uart_event(map, BSP_UART_EVENT_ERROR, nullptr, 0U, BSP_ERROR);
        }
        if ((status & DMA_CHANNEL_STATUS_HALF_TC) != 0U) {
            emit_uart_event(map, BSP_UART_EVENT_TX_HALF, nullptr, 0U, BSP_OK);
        }
        if ((status & DMA_CHANNEL_STATUS_TC) != 0U) {
            stop_uart_dma_channel(dma, dma.tx_ch);
            map->tx_dma_active = false;
            emit_uart_event(map, BSP_UART_EVENT_TX_DONE, nullptr, 0U, BSP_OK);
        }
    }
}

#define BSP_UART_ISR_ITEM(name, base_addr, irq_num, features) \
    SDK_DECLARE_EXT_ISR_M(irq_num, hpm_bsp_##name##_isr) \
    void hpm_bsp_##name##_isr(void)                      \
    {                                                    \
        bsp_uart_irq_handler(name);                      \
    }

BSP_UART_LIST(BSP_UART_ISR_ITEM)

#undef BSP_UART_ISR_ITEM

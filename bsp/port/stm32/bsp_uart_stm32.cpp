#include "bsp_uart.h"

#include <cstdint>
#include <cstring>

#include "stm32_support.hpp"

extern "C" {
#include "usart.h"
}

typedef struct {
    USART_TypeDef *base;
    IRQn_Type irq;
    std::uint32_t features;
    UART_HandleTypeDef *handle;
    void (*init_fn)(void);
    bsp_transfer_mode_t transfer_mode;
    bool initialized;
    uint8_t *rx_buffer;
    uint16_t rx_length;
    bsp_uart_event_callback_t event_callback;
    void *event_callback_arg;
} uart_map_t;

static void attach_uart_bindings();
static DMA_HandleTypeDef s_uart1_dma_rx;
static DMA_HandleTypeDef s_uart1_dma_tx;
static bool s_uart1_dma_configured = false;

static void uart_init_fn_debug()
{
    MX_USART1_UART_Init();
}

#define BSP_UART_MAP_ITEM(name, base, irq, features) \
    { base, irq, static_cast<std::uint32_t>(features), nullptr, nullptr, BSP_TRANSFER_MODE_AUTO, false, nullptr, 0U, nullptr, nullptr },
static uart_map_t s_uart_map[BSP_UART_MAX] = {
    BSP_UART_LIST(BSP_UART_MAP_ITEM)
};
#undef BSP_UART_MAP_ITEM

static bool uart_has_feature(const uart_map_t &map, std::uint32_t feature)
{
    return (map.features & feature) != 0U;
}

static bool uart_parity_supported(const uart_map_t &map, bsp_uart_parity_t parity)
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

static bool uart_stop_bits_supported(const uart_map_t &map, bsp_uart_stop_bits_t stop_bits)
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

static bool uart_word_length_supported(const uart_map_t &map, bsp_uart_word_length_t word_length)
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

static int32_t validate_uart_init_mode(const uart_map_t &map, bsp_transfer_mode_t transfer_mode)
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
        return (uart_has_feature(map, BSP_UART_FEATURE_DMA_TX) || uart_has_feature(map, BSP_UART_FEATURE_DMA_RX))
                   ? BSP_OK
                   : BSP_ERROR_UNSUPPORTED;
    default:
        return BSP_ERROR_PARAM;
    }
}

static bool uart_dma_tx_available(const uart_map_t &map)
{
    return uart_has_feature(map, BSP_UART_FEATURE_DMA_TX) && (map.handle != nullptr) && (map.handle->hdmatx != nullptr);
}

static bool uart_dma_rx_available(const uart_map_t &map)
{
    return uart_has_feature(map, BSP_UART_FEATURE_DMA_RX) && (map.handle != nullptr) && (map.handle->hdmarx != nullptr);
}

static int32_t resolve_uart_tx_mode(const uart_map_t &map, bsp_transfer_mode_t *out_mode)
{
    switch (map.transfer_mode) {
    case BSP_TRANSFER_MODE_AUTO:
        if (uart_dma_tx_available(map)) {
            *out_mode = BSP_TRANSFER_MODE_DMA;
            return BSP_OK;
        }
        if (uart_has_feature(map, BSP_UART_FEATURE_INTERRUPT_TX)) {
            *out_mode = BSP_TRANSFER_MODE_INTERRUPT;
            return BSP_OK;
        }
        return BSP_ERROR_UNSUPPORTED;
    case BSP_TRANSFER_MODE_DMA:
        if (uart_dma_tx_available(map)) {
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

static int32_t resolve_uart_rx_mode(const uart_map_t &map, bsp_transfer_mode_t *out_mode)
{
    switch (map.transfer_mode) {
    case BSP_TRANSFER_MODE_AUTO:
        if (uart_dma_rx_available(map)) {
            *out_mode = BSP_TRANSFER_MODE_DMA;
            return BSP_OK;
        }
        if (uart_has_feature(map, BSP_UART_FEATURE_INTERRUPT_RX)) {
            *out_mode = BSP_TRANSFER_MODE_INTERRUPT;
            return BSP_OK;
        }
        return BSP_ERROR_UNSUPPORTED;
    case BSP_TRANSFER_MODE_DMA:
        if (uart_dma_rx_available(map)) {
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

static void emit_uart_event(UART_HandleTypeDef *handle,
                            bsp_uart_event_t event,
                            uint8_t *data,
                            uint16_t length,
                            int32_t status)
{
    attach_uart_bindings();
    for (std::uint32_t index = 0; index < BSP_UART_MAX; ++index) {
        auto &map = s_uart_map[index];
        if ((map.handle == handle) && (map.event_callback != nullptr)) {
            map.event_callback(event, data, length, status, map.event_callback_arg);
            return;
        }
    }
}

static void attach_uart_bindings()
{
    static bool attached = false;
    if (attached) {
        return;
    }

    for (std::uint32_t index = 0; index < BSP_UART_MAX; ++index) {
        auto &map = s_uart_map[index];
        switch (static_cast<bsp_uart_instance_t>(index)) {
        case BSP_UART_DEBUG:
            map.handle = &huart1;
            map.init_fn = uart_init_fn_debug;
            break;
        default:
            break;
        }
    }

    attached = true;
}

static bool configure_uart1_dma(uart_map_t &map)
{
    if (map.handle == nullptr) {
        return false;
    }
    if (s_uart1_dma_configured) {
        return (map.handle->hdmarx == &s_uart1_dma_rx) && (map.handle->hdmatx == &s_uart1_dma_tx);
    }

    std::memset(&s_uart1_dma_rx, 0, sizeof(s_uart1_dma_rx));
    s_uart1_dma_rx.Instance = DMA1_Stream2;
    s_uart1_dma_rx.Init.Request = DMA_REQUEST_USART1_RX;
    s_uart1_dma_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    s_uart1_dma_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    s_uart1_dma_rx.Init.MemInc = DMA_MINC_ENABLE;
    s_uart1_dma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    s_uart1_dma_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    s_uart1_dma_rx.Init.Mode = DMA_NORMAL;
    s_uart1_dma_rx.Init.Priority = DMA_PRIORITY_LOW;
    s_uart1_dma_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&s_uart1_dma_rx) != HAL_OK) {
        return false;
    }

    std::memset(&s_uart1_dma_tx, 0, sizeof(s_uart1_dma_tx));
    s_uart1_dma_tx.Instance = DMA1_Stream4;
    s_uart1_dma_tx.Init.Request = DMA_REQUEST_USART1_TX;
    s_uart1_dma_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    s_uart1_dma_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    s_uart1_dma_tx.Init.MemInc = DMA_MINC_ENABLE;
    s_uart1_dma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    s_uart1_dma_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    s_uart1_dma_tx.Init.Mode = DMA_NORMAL;
    s_uart1_dma_tx.Init.Priority = DMA_PRIORITY_LOW;
    s_uart1_dma_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&s_uart1_dma_tx) != HAL_OK) {
        return false;
    }

    __HAL_LINKDMA(map.handle, hdmarx, s_uart1_dma_rx);
    __HAL_LINKDMA(map.handle, hdmatx, s_uart1_dma_tx);
    HAL_NVIC_SetPriority(DMA1_Stream2_IRQn, BSP_IRQ_PRIORITY_DEFAULT, 0U);
    HAL_NVIC_EnableIRQ(DMA1_Stream2_IRQn);
    HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, BSP_IRQ_PRIORITY_DEFAULT, 0U);
    HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);
    s_uart1_dma_configured = true;
    return true;
}

static int32_t ensure_uart_initialized(bsp_uart_instance_t uart)
{
    if (uart >= BSP_UART_MAX) {
        return BSP_ERROR_PARAM;
    }

    attach_uart_bindings();
    auto &map = s_uart_map[uart];
    if (map.handle == nullptr || map.init_fn == nullptr) {
        return BSP_ERROR_PARAM;
    }

    if (!map.initialized) {
        stm32_dma_ensure_initialized();
        map.init_fn();
        if ((uart == BSP_UART_DEBUG) &&
            (uart_has_feature(map, BSP_UART_FEATURE_DMA_RX) || uart_has_feature(map, BSP_UART_FEATURE_DMA_TX))) {
            if (!configure_uart1_dma(map)) {
                return BSP_ERROR_UNSUPPORTED;
            }
        }
        HAL_NVIC_SetPriority(map.irq, BSP_IRQ_PRIORITY_DEFAULT, 0U);
        HAL_NVIC_EnableIRQ(map.irq);
        map.initialized = true;
    }

    return BSP_OK;
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

    attach_uart_bindings();
    auto &map = s_uart_map[uart];
    if ((baudrate != 0U) && !uart_has_feature(map, BSP_UART_FEATURE_CONFIGURABLE_BAUD)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (!uart_parity_supported(map, parity) || !uart_stop_bits_supported(map, stop_bits) || !uart_word_length_supported(map, word_length)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    const auto mode_status = validate_uart_init_mode(map, transfer_mode);
    if (mode_status != BSP_OK) {
        return mode_status;
    }

    const auto ensure_status = ensure_uart_initialized(uart);
    if (ensure_status != BSP_OK) {
        return ensure_status;
    }

    if (baudrate != 0U) {
        map.handle->Init.BaudRate = baudrate;
    }

    switch (parity) {
    case BSP_UART_PARITY_ODD:
        map.handle->Init.Parity = UART_PARITY_ODD;
        break;
    case BSP_UART_PARITY_EVEN:
        map.handle->Init.Parity = UART_PARITY_EVEN;
        break;
    case BSP_UART_PARITY_NONE:
        map.handle->Init.Parity = UART_PARITY_NONE;
        break;
    default:
        return BSP_ERROR_UNSUPPORTED;
    }

    switch (stop_bits) {
    case BSP_UART_STOP_BITS_1_5:
        map.handle->Init.StopBits = UART_STOPBITS_1_5;
        break;
    case BSP_UART_STOP_BITS_2:
        map.handle->Init.StopBits = UART_STOPBITS_2;
        break;
    case BSP_UART_STOP_BITS_1:
        map.handle->Init.StopBits = UART_STOPBITS_1;
        break;
    default:
        return BSP_ERROR_UNSUPPORTED;
    }

    if (word_length == BSP_UART_WORD_LENGTH_8) {
        map.handle->Init.WordLength = (parity == BSP_UART_PARITY_NONE) ? UART_WORDLENGTH_8B : UART_WORDLENGTH_9B;
    } else if (word_length == BSP_UART_WORD_LENGTH_7) {
#ifdef UART_WORDLENGTH_7B
        map.handle->Init.WordLength = (parity == BSP_UART_PARITY_NONE) ? UART_WORDLENGTH_7B : UART_WORDLENGTH_8B;
#else
        return BSP_ERROR_UNSUPPORTED;
#endif
    } else {
        return BSP_ERROR_UNSUPPORTED;
    }

    map.transfer_mode = transfer_mode;
    HAL_NVIC_SetPriority(map.irq, (irq_priority == 0U) ? BSP_IRQ_PRIORITY_DEFAULT : irq_priority, 0U);
    return (HAL_UART_Init(map.handle) == HAL_OK) ? BSP_OK : BSP_ERROR;
}

int32_t bsp_uart_send(bsp_uart_instance_t uart, uint8_t *data, uint16_t len, uint32_t timeout)
{
    if ((uart >= BSP_UART_MAX) || (data == nullptr) || (len == 0U)) {
        return BSP_ERROR_PARAM;
    }
    const auto ensure_status = ensure_uart_initialized(uart);
    if (ensure_status != BSP_OK) {
        return ensure_status;
    }
    if (!uart_has_feature(s_uart_map[uart], BSP_UART_FEATURE_BLOCKING_TX)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    const uint32_t effective_timeout = (timeout == 0U) ? HAL_MAX_DELAY : timeout;
    return (HAL_UART_Transmit(s_uart_map[uart].handle, data, len, effective_timeout) == HAL_OK) ? BSP_OK : BSP_ERROR_TIMEOUT;
}

int32_t bsp_uart_send_async(bsp_uart_instance_t uart, const uint8_t *data, uint16_t len)
{
    if ((uart >= BSP_UART_MAX) || (data == nullptr) || (len == 0U)) {
        return BSP_ERROR_PARAM;
    }
    const auto ensure_status = ensure_uart_initialized(uart);
    if (ensure_status != BSP_OK) {
        return ensure_status;
    }

    auto &map = s_uart_map[uart];
    bsp_transfer_mode_t tx_mode = BSP_TRANSFER_MODE_AUTO;
    const auto mode_status = resolve_uart_tx_mode(map, &tx_mode);
    if (mode_status != BSP_OK) {
        return mode_status;
    }

    auto *handle = map.handle;
    const auto status = (tx_mode == BSP_TRANSFER_MODE_DMA)
                            ? HAL_UART_Transmit_DMA(handle, const_cast<uint8_t *>(data), len)
                            : HAL_UART_Transmit_IT(handle, const_cast<uint8_t *>(data), len);
    return (status == HAL_OK) ? BSP_OK : BSP_ERROR_BUSY;
}

int32_t bsp_uart_receive_it(bsp_uart_instance_t uart, uint8_t *rx_buf, uint16_t max_len)
{
    if ((uart >= BSP_UART_MAX) || (rx_buf == nullptr) || (max_len == 0U)) {
        return BSP_ERROR_PARAM;
    }
    const auto ensure_status = ensure_uart_initialized(uart);
    if (ensure_status != BSP_OK) {
        return ensure_status;
    }

    auto &map = s_uart_map[uart];
    bsp_transfer_mode_t rx_mode = BSP_TRANSFER_MODE_AUTO;
    const auto mode_status = resolve_uart_rx_mode(map, &rx_mode);
    if (mode_status != BSP_OK) {
        return mode_status;
    }

    map.rx_buffer = rx_buf;
    map.rx_length = max_len;

    HAL_StatusTypeDef status = HAL_ERROR;
    if (rx_mode == BSP_TRANSFER_MODE_DMA) {
        status = HAL_UARTEx_ReceiveToIdle_DMA(map.handle, rx_buf, max_len);
    } else {
        status = HAL_UART_Receive_IT(map.handle, rx_buf, max_len);
    }

    return (status == HAL_OK) ? BSP_OK : BSP_ERROR_BUSY;
}

int32_t bsp_uart_register_event_callback(bsp_uart_instance_t uart, bsp_uart_event_callback_t callback, void *arg)
{
    if (uart >= BSP_UART_MAX) {
        return BSP_ERROR_PARAM;
    }

    attach_uart_bindings();
    s_uart_map[uart].event_callback = callback;
    s_uart_map[uart].event_callback_arg = arg;
    return BSP_OK;
}

static void emit_uart_callback(UART_HandleTypeDef *handle, uint16_t length)
{
    attach_uart_bindings();
    for (std::uint32_t index = 0; index < BSP_UART_MAX; ++index) {
        auto &map = s_uart_map[index];
        if ((map.handle == handle) && (map.event_callback != nullptr) && (map.rx_buffer != nullptr)) {
            map.event_callback(BSP_UART_EVENT_RX_DONE, map.rx_buffer, length, BSP_OK, map.event_callback_arg);
            return;
        }
    }
}

extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    attach_uart_bindings();
    for (std::uint32_t index = 0; index < BSP_UART_MAX; ++index) {
        const auto &map = s_uart_map[index];
        if ((map.handle == huart) && (map.rx_length > 0U)) {
            emit_uart_callback(huart, map.rx_length);
            emit_uart_event(huart, BSP_UART_EVENT_RX_DONE, map.rx_buffer, map.rx_length, BSP_OK);
            return;
        }
    }
}

extern "C" void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
    const auto event = HAL_UARTEx_GetRxEventType(huart);
    switch (event) {
    case HAL_UART_RXEVENT_HT:
        emit_uart_event(huart, BSP_UART_EVENT_RX_HALF, nullptr, size, BSP_OK);
        break;
    case HAL_UART_RXEVENT_IDLE:
        emit_uart_callback(huart, size);
        emit_uart_event(huart, BSP_UART_EVENT_RX_IDLE, nullptr, size, BSP_OK);
        break;
    case HAL_UART_RXEVENT_TC:
    default:
        emit_uart_callback(huart, size);
        emit_uart_event(huart, BSP_UART_EVENT_RX_DONE, nullptr, size, BSP_OK);
        break;
    }
}

extern "C" void HAL_UART_TxHalfCpltCallback(UART_HandleTypeDef *huart)
{
    emit_uart_event(huart, BSP_UART_EVENT_TX_HALF, nullptr, 0U, BSP_OK);
}

extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    attach_uart_bindings();
    for (std::uint32_t index = 0; index < BSP_UART_MAX; ++index) {
        const auto &map = s_uart_map[index];
        if (map.handle == huart) {
            emit_uart_event(huart, BSP_UART_EVENT_TX_DONE, nullptr, 0U, BSP_OK);
            return;
        }
    }
}

extern "C" void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    attach_uart_bindings();
    for (std::uint32_t index = 0; index < BSP_UART_MAX; ++index) {
        const auto &map = s_uart_map[index];
        if (map.handle == huart) {
            emit_uart_event(huart, BSP_UART_EVENT_ERROR, map.rx_buffer, 0U, BSP_ERROR);
            return;
        }
    }
}

extern "C" void Handle_IDLE_Callback(void)
{
    attach_uart_bindings();
    auto &map = s_uart_map[BSP_UART_DEBUG];
    if ((map.handle == nullptr) || (map.handle->hdmarx == nullptr) || (map.event_callback == nullptr) || (map.rx_buffer == nullptr)) {
        return;
    }

    const auto remaining = __HAL_DMA_GET_COUNTER(map.handle->hdmarx);
    const auto received = static_cast<uint16_t>(map.rx_length - remaining);
    emit_uart_event(map.handle, BSP_UART_EVENT_RX_IDLE, map.rx_buffer, received, BSP_OK);
}

extern "C" void DMA1_Stream2_IRQHandler(void)
{
    if (s_uart1_dma_configured) {
        HAL_DMA_IRQHandler(&s_uart1_dma_rx);
    }
}

extern "C" void DMA1_Stream4_IRQHandler(void)
{
    if (s_uart1_dma_configured) {
        HAL_DMA_IRQHandler(&s_uart1_dma_tx);
    }
}

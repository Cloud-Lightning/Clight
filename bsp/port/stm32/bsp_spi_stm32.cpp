#include "bsp_spi.h"

#include <cstdint>
#include <cstring>

#include "stm32_support.hpp"

extern "C" {
#include "spi.h"
}

typedef struct {
    SPI_TypeDef *base;
    IRQn_Type irq;
    bool has_pinmux;
    std::uint32_t features;
    SPI_HandleTypeDef *handle;
    void (*init_fn)(void);
    bsp_transfer_mode_t transfer_mode;
    bool initialized;
    bool async_in_flight;
    bsp_spi_event_callback_t event_callback;
    void *event_callback_arg;
    bsp_spi_event_t pending_event;
} spi_map_t;

static void spi_init_fn_main()
{
    MX_SPI2_Init();
}

static void spi_init_fn_aux()
{
    MX_SPI6_Init();
}

#define BSP_SPI_MAP_ITEM(name, base, irq, clk, has_pinmux, features) \
    { base, irq, (has_pinmux) != 0U, static_cast<std::uint32_t>(features), nullptr, nullptr, BSP_TRANSFER_MODE_AUTO, false, false, nullptr, nullptr, BSP_SPI_EVENT_TRANSFER },
static spi_map_t s_spi_map[BSP_SPI_MAX] = {
    BSP_SPI_LIST(BSP_SPI_MAP_ITEM)
};
#undef BSP_SPI_MAP_ITEM

static bool spi_has_feature(const spi_map_t &map, std::uint32_t feature)
{
    return (map.features & feature) != 0U;
}

static int32_t validate_spi_init_mode(const spi_map_t &map, bsp_transfer_mode_t transfer_mode)
{
    switch (transfer_mode) {
    case BSP_TRANSFER_MODE_AUTO:
        return BSP_OK;
    case BSP_TRANSFER_MODE_BLOCKING:
        return spi_has_feature(map, BSP_SPI_FEATURE_BLOCKING) ? BSP_OK : BSP_ERROR_UNSUPPORTED;
    case BSP_TRANSFER_MODE_INTERRUPT:
        return spi_has_feature(map, BSP_SPI_FEATURE_INTERRUPT) ? BSP_OK : BSP_ERROR_UNSUPPORTED;
    case BSP_TRANSFER_MODE_DMA:
        return ((spi_has_feature(map, BSP_SPI_FEATURE_DMA_TX)) || (spi_has_feature(map, BSP_SPI_FEATURE_DMA_RX)))
                   ? BSP_OK
                   : BSP_ERROR_UNSUPPORTED;
    default:
        return BSP_ERROR_PARAM;
    }
}

static int32_t validate_spi_direction(const spi_map_t &map, bool has_tx, bool has_rx)
{
    if (has_tx && has_rx) {
        return spi_has_feature(map, BSP_SPI_FEATURE_FULL_DUPLEX) ? BSP_OK : BSP_ERROR_UNSUPPORTED;
    }
    if (has_tx) {
        return (spi_has_feature(map, BSP_SPI_FEATURE_TX_ONLY) || spi_has_feature(map, BSP_SPI_FEATURE_FULL_DUPLEX))
                   ? BSP_OK
                   : BSP_ERROR_UNSUPPORTED;
    }
    if (has_rx) {
        return (spi_has_feature(map, BSP_SPI_FEATURE_RX_ONLY) || spi_has_feature(map, BSP_SPI_FEATURE_FULL_DUPLEX))
                   ? BSP_OK
                   : BSP_ERROR_UNSUPPORTED;
    }
    return BSP_ERROR_PARAM;
}

static bool spi_dma_available(const spi_map_t &map, bool has_tx, bool has_rx)
{
    if (map.handle == nullptr) {
        return false;
    }
    if (has_tx && (!spi_has_feature(map, BSP_SPI_FEATURE_DMA_TX) || (map.handle->hdmatx == nullptr))) {
        return false;
    }
    if (has_rx && (!spi_has_feature(map, BSP_SPI_FEATURE_DMA_RX) || (map.handle->hdmarx == nullptr))) {
        return false;
    }
    return has_tx || has_rx;
}

static int32_t resolve_spi_async_mode(const spi_map_t &map, bool has_tx, bool has_rx, bsp_transfer_mode_t *out_mode)
{
    switch (map.transfer_mode) {
    case BSP_TRANSFER_MODE_AUTO:
        if (spi_dma_available(map, has_tx, has_rx)) {
            *out_mode = BSP_TRANSFER_MODE_DMA;
            return BSP_OK;
        }
        if (spi_has_feature(map, BSP_SPI_FEATURE_INTERRUPT)) {
            *out_mode = BSP_TRANSFER_MODE_INTERRUPT;
            return BSP_OK;
        }
        return BSP_ERROR_UNSUPPORTED;
    case BSP_TRANSFER_MODE_DMA:
        if (spi_dma_available(map, has_tx, has_rx)) {
            *out_mode = BSP_TRANSFER_MODE_DMA;
            return BSP_OK;
        }
        return BSP_ERROR_UNSUPPORTED;
    case BSP_TRANSFER_MODE_INTERRUPT:
        if (spi_has_feature(map, BSP_SPI_FEATURE_INTERRUPT)) {
            *out_mode = BSP_TRANSFER_MODE_INTERRUPT;
            return BSP_OK;
        }
        return BSP_ERROR_UNSUPPORTED;
    default:
        return BSP_ERROR_UNSUPPORTED;
    }
}

static std::uint32_t spi_source_clock_hz(SPI_TypeDef *base)
{
#if defined(SPI2) && defined(RCC_PERIPHCLK_SPI2)
    if (base == SPI2) {
        return HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_SPI2);
    }
#endif
#if defined(SPI6) && defined(RCC_PERIPHCLK_SPI6)
    if (base == SPI6) {
        return HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_SPI6);
    }
#endif
    (void) base;
    return 0U;
}

static std::uint32_t select_spi_prescaler(std::uint32_t source_hz, std::uint32_t target_hz)
{
    struct prescaler_entry_t {
        std::uint32_t div;
        std::uint32_t value;
    };

    static constexpr prescaler_entry_t kPrescalers[] = {
        {2U, SPI_BAUDRATEPRESCALER_2},
        {4U, SPI_BAUDRATEPRESCALER_4},
        {8U, SPI_BAUDRATEPRESCALER_8},
        {16U, SPI_BAUDRATEPRESCALER_16},
        {32U, SPI_BAUDRATEPRESCALER_32},
        {64U, SPI_BAUDRATEPRESCALER_64},
        {128U, SPI_BAUDRATEPRESCALER_128},
        {256U, SPI_BAUDRATEPRESCALER_256},
    };

    for (const auto &entry : kPrescalers) {
        if ((source_hz / entry.div) <= target_hz) {
            return entry.value;
        }
    }
    return SPI_BAUDRATEPRESCALER_256;
}

static void attach_spi_bindings()
{
    static bool attached = false;
    if (attached) {
        return;
    }

    s_spi_map[BSP_SPI_MAIN].handle = &hspi2;
    s_spi_map[BSP_SPI_MAIN].init_fn = spi_init_fn_main;
    s_spi_map[BSP_SPI_AUX].handle = &hspi6;
    s_spi_map[BSP_SPI_AUX].init_fn = spi_init_fn_aux;
    attached = true;
}

static int32_t ensure_spi_initialized(bsp_spi_instance_t spi)
{
    if (spi >= BSP_SPI_MAX) {
        return BSP_ERROR_PARAM;
    }

    attach_spi_bindings();
    auto &map = s_spi_map[spi];
    if (!map.has_pinmux) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (map.handle == nullptr || map.init_fn == nullptr) {
        return BSP_ERROR_PARAM;
    }
    if (!map.initialized) {
        stm32_dma_ensure_initialized();
        map.init_fn();
        HAL_NVIC_SetPriority(map.irq, BSP_IRQ_PRIORITY_DEFAULT, 0U);
        HAL_NVIC_EnableIRQ(map.irq);
        map.initialized = true;
    }
    return BSP_OK;
}

static int32_t transfer_with_dummy_rx(SPI_HandleTypeDef *handle, uint8_t *rx_data, uint32_t size)
{
    std::uint8_t dummy_tx[32];
    std::memset(dummy_tx, 0xFF, sizeof(dummy_tx));

    uint32_t offset = 0U;
    while (offset < size) {
        const auto chunk = static_cast<uint16_t>(((size - offset) > sizeof(dummy_tx)) ? sizeof(dummy_tx) : (size - offset));
        if (HAL_SPI_TransmitReceive(handle, dummy_tx, rx_data + offset, chunk, HAL_MAX_DELAY) != HAL_OK) {
            return BSP_ERROR_TIMEOUT;
        }
        offset += chunk;
    }

    return BSP_OK;
}

int32_t bsp_spi_init(bsp_spi_instance_t spi,
                     bsp_spi_mode_t mode,
                     uint32_t frequency_hz,
                     bsp_spi_bit_order_t bit_order,
                     bsp_transfer_mode_t transfer_mode,
                     uint8_t irq_priority)
{
    if (spi >= BSP_SPI_MAX) {
        return BSP_ERROR_PARAM;
    }

    attach_spi_bindings();
    auto &map = s_spi_map[spi];
    const auto init_mode_status = validate_spi_init_mode(map, transfer_mode);
    if (init_mode_status != BSP_OK) {
        return init_mode_status;
    }
    if ((bit_order == BSP_SPI_BIT_ORDER_MSB_FIRST) && !spi_has_feature(map, BSP_SPI_FEATURE_MSB_FIRST)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if ((bit_order == BSP_SPI_BIT_ORDER_LSB_FIRST) && !spi_has_feature(map, BSP_SPI_FEATURE_LSB_FIRST)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if ((frequency_hz != 0U) && !spi_has_feature(map, BSP_SPI_FEATURE_CONFIGURABLE_CLOCK)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    const auto ensure_status = ensure_spi_initialized(spi);
    if (ensure_status != BSP_OK) {
        return ensure_status;
    }

    auto *handle = map.handle;
    switch (mode) {
    case BSP_SPI_MODE_0:
        handle->Init.CLKPolarity = SPI_POLARITY_LOW;
        handle->Init.CLKPhase = SPI_PHASE_1EDGE;
        break;
    case BSP_SPI_MODE_1:
        handle->Init.CLKPolarity = SPI_POLARITY_LOW;
        handle->Init.CLKPhase = SPI_PHASE_2EDGE;
        break;
    case BSP_SPI_MODE_2:
        handle->Init.CLKPolarity = SPI_POLARITY_HIGH;
        handle->Init.CLKPhase = SPI_PHASE_1EDGE;
        break;
    case BSP_SPI_MODE_3:
    default:
        handle->Init.CLKPolarity = SPI_POLARITY_HIGH;
        handle->Init.CLKPhase = SPI_PHASE_2EDGE;
        break;
    }
    handle->Init.FirstBit = (bit_order == BSP_SPI_BIT_ORDER_LSB_FIRST) ? SPI_FIRSTBIT_LSB : SPI_FIRSTBIT_MSB;
    if (frequency_hz != 0U) {
        const auto source_hz = spi_source_clock_hz(map.base);
        if (source_hz == 0U) {
            return BSP_ERROR_UNSUPPORTED;
        }
        handle->Init.BaudRatePrescaler = select_spi_prescaler(source_hz, frequency_hz);
    }
    map.transfer_mode = transfer_mode;
    HAL_NVIC_SetPriority(map.irq, irq_priority, 0U);
    return (HAL_SPI_Init(handle) == HAL_OK) ? BSP_OK : BSP_ERROR;
}

int32_t bsp_spi_transfer(bsp_spi_instance_t spi, const uint8_t *tx_data, uint8_t *rx_data, uint32_t size)
{
    if ((spi >= BSP_SPI_MAX) || (size == 0U) || ((tx_data == nullptr) && (rx_data == nullptr))) {
        return BSP_ERROR_PARAM;
    }
    const auto ensure_status = ensure_spi_initialized(spi);
    if (ensure_status != BSP_OK) {
        return ensure_status;
    }

    auto &map = s_spi_map[spi];
    const auto direction_status = validate_spi_direction(map, tx_data != nullptr, rx_data != nullptr);
    if (direction_status != BSP_OK) {
        return direction_status;
    }
    if (!spi_has_feature(map, BSP_SPI_FEATURE_BLOCKING)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    auto *handle = map.handle;
    if (tx_data != nullptr && rx_data != nullptr) {
        return (HAL_SPI_TransmitReceive(handle, const_cast<uint8_t *>(tx_data), rx_data, static_cast<uint16_t>(size), HAL_MAX_DELAY) == HAL_OK)
                   ? BSP_OK
                   : BSP_ERROR_TIMEOUT;
    }
    if (tx_data != nullptr) {
        return (HAL_SPI_Transmit(handle, const_cast<uint8_t *>(tx_data), static_cast<uint16_t>(size), HAL_MAX_DELAY) == HAL_OK)
                   ? BSP_OK
                   : BSP_ERROR_TIMEOUT;
    }

    return transfer_with_dummy_rx(handle, rx_data, size);
}

int32_t bsp_spi_transfer_async(bsp_spi_instance_t spi, const uint8_t *tx_data, uint8_t *rx_data, uint32_t size)
{
    if ((spi >= BSP_SPI_MAX) || (size == 0U) || ((tx_data == nullptr) && (rx_data == nullptr))) {
        return BSP_ERROR_PARAM;
    }
    const auto ensure_status = ensure_spi_initialized(spi);
    if (ensure_status != BSP_OK) {
        return ensure_status;
    }

    auto &map = s_spi_map[spi];
    const bool has_tx = tx_data != nullptr;
    const bool has_rx = rx_data != nullptr;
    const auto direction_status = validate_spi_direction(map, has_tx, has_rx);
    if (direction_status != BSP_OK) {
        return direction_status;
    }

    bsp_transfer_mode_t async_mode = BSP_TRANSFER_MODE_AUTO;
    const auto mode_status = resolve_spi_async_mode(map, has_tx, has_rx, &async_mode);
    if (mode_status != BSP_OK) {
        return mode_status;
    }

    if (map.async_in_flight) {
        return BSP_ERROR_BUSY;
    }

    HAL_StatusTypeDef status = HAL_ERROR;
    if ((tx_data != nullptr) && (rx_data != nullptr)) {
        map.pending_event = BSP_SPI_EVENT_TRANSFER;
        status = (async_mode == BSP_TRANSFER_MODE_DMA)
                     ? HAL_SPI_TransmitReceive_DMA(map.handle, const_cast<uint8_t *>(tx_data), rx_data, static_cast<uint16_t>(size))
                     : HAL_SPI_TransmitReceive_IT(map.handle, const_cast<uint8_t *>(tx_data), rx_data, static_cast<uint16_t>(size));
    } else if (tx_data != nullptr) {
        map.pending_event = BSP_SPI_EVENT_WRITE;
        status = (async_mode == BSP_TRANSFER_MODE_DMA)
                     ? HAL_SPI_Transmit_DMA(map.handle, const_cast<uint8_t *>(tx_data), static_cast<uint16_t>(size))
                     : HAL_SPI_Transmit_IT(map.handle, const_cast<uint8_t *>(tx_data), static_cast<uint16_t>(size));
    } else {
        map.pending_event = BSP_SPI_EVENT_READ;
        status = (async_mode == BSP_TRANSFER_MODE_DMA)
                     ? HAL_SPI_Receive_DMA(map.handle, rx_data, static_cast<uint16_t>(size))
                     : HAL_SPI_Receive_IT(map.handle, rx_data, static_cast<uint16_t>(size));
    }

    if (status != HAL_OK) {
        return (status == HAL_BUSY) ? BSP_ERROR_BUSY : BSP_ERROR;
    }

    map.async_in_flight = true;
    return BSP_OK;
}

int32_t bsp_spi_register_event_callback(bsp_spi_instance_t spi, bsp_spi_event_callback_t callback, void *arg)
{
    if (spi >= BSP_SPI_MAX) {
        return BSP_ERROR_PARAM;
    }

    s_spi_map[spi].event_callback = callback;
    s_spi_map[spi].event_callback_arg = arg;
    return BSP_OK;
}

static void emit_spi_callback(SPI_HandleTypeDef *handle, int32_t status)
{
    attach_spi_bindings();
    for (std::uint32_t index = 0; index < BSP_SPI_MAX; ++index) {
        auto &map = s_spi_map[index];
        if (map.handle == handle) {
            map.async_in_flight = false;
            if (map.event_callback != nullptr) {
                map.event_callback(map.pending_event, status, map.event_callback_arg);
            }
            break;
        }
    }
}

extern "C" void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    emit_spi_callback(hspi, BSP_OK);
}

extern "C" void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    emit_spi_callback(hspi, BSP_OK);
}

extern "C" void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    emit_spi_callback(hspi, BSP_OK);
}

extern "C" void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    emit_spi_callback(hspi, BSP_ERROR);
}

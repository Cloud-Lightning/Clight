/**
 * @file bsp_spi_hpm.cpp
 * @brief HPM platform implementation for BSP SPI interface
 */

#include "bsp_spi.h"
#include "board.h"

extern "C" {
#include "hpm_dmav2_drv.h"
#include "hpm_dmamux_drv.h"
#include "hpm_spi_drv.h"
#include "hpm_interrupt.h"
}

typedef struct {
    SPI_Type *base;
    uint32_t irq;
    uint32_t clock_name;
    bool has_pinmux;
    uint32_t features;
    bool initialized;
    const uint8_t *tx_data;
    uint8_t *rx_data;
    uint32_t tx_remaining;
    uint32_t rx_remaining;
    uint8_t data_len_in_bytes;
    bsp_transfer_mode_t transfer_mode;
    uint8_t irq_priority;
    bool async_in_flight;
    bool dma_tx_active;
    bool dma_rx_active;
    spi_control_config_t async_control;
    bsp_spi_event_callback_t event_callback;
    void *event_callback_arg;
} spi_instance_map_t;

#define BSP_SPI_MAP_ITEM(name, base_addr, irq_num, clk_name, has_pinmux, features) \
    { base_addr, irq_num, clk_name, (has_pinmux) != 0U, static_cast<uint32_t>(features), false, nullptr, nullptr, 0U, 0U, 1U, BSP_TRANSFER_MODE_AUTO, BSP_IRQ_PRIORITY_DEFAULT, false, false, false, {}, nullptr, nullptr },
static spi_instance_map_t s_spi_map[BSP_SPI_MAX] = {
    BSP_SPI_LIST(BSP_SPI_MAP_ITEM)
};
#undef BSP_SPI_MAP_ITEM

typedef struct {
    DMA_Type *base;
    uint32_t irq;
    DMAMUX_Type *dmamux;
    uint32_t tx_req;
    uint32_t rx_req;
    uint8_t tx_ch;
    uint8_t rx_ch;
} spi_dma_resource_t;

#define BSP_SPI_DMA_MAP_ITEM(name, base_addr, irq_num, clk_name, has_pinmux, features) \
    { name##_DMA_CONTROLLER, name##_DMA_IRQ, name##_DMAMUX, name##_DMA_TX_REQ, name##_DMA_RX_REQ, name##_DMA_TX_CH, name##_DMA_RX_CH },
static const spi_dma_resource_t s_spi_dma_map[BSP_SPI_MAX] = {
    BSP_SPI_LIST(BSP_SPI_DMA_MAP_ITEM)
};
#undef BSP_SPI_DMA_MAP_ITEM

static uint8_t normalize_irq_priority(uint8_t irq_priority)
{
    return (irq_priority == 0U) ? BSP_IRQ_PRIORITY_DEFAULT : irq_priority;
}

static bool spi_has_feature(const spi_instance_map_t &map, uint32_t feature)
{
    return (map.features & feature) != 0U;
}

static bool spi_dma_resource_present(const spi_dma_resource_t &dma)
{
    return (dma.base != nullptr) && (dma.dmamux != nullptr);
}

static int32_t validate_spi_init_mode(const spi_instance_map_t &map,
                                      const spi_dma_resource_t &dma,
                                      bsp_transfer_mode_t transfer_mode)
{
    switch (transfer_mode) {
    case BSP_TRANSFER_MODE_AUTO:
        return BSP_OK;
    case BSP_TRANSFER_MODE_BLOCKING:
        return spi_has_feature(map, BSP_SPI_FEATURE_BLOCKING) ? BSP_OK : BSP_ERROR_UNSUPPORTED;
    case BSP_TRANSFER_MODE_INTERRUPT:
        return spi_has_feature(map, BSP_SPI_FEATURE_INTERRUPT) ? BSP_OK : BSP_ERROR_UNSUPPORTED;
    case BSP_TRANSFER_MODE_DMA:
        return ((spi_has_feature(map, BSP_SPI_FEATURE_DMA_TX) || spi_has_feature(map, BSP_SPI_FEATURE_DMA_RX)) &&
                spi_dma_resource_present(dma))
                   ? BSP_OK
                   : BSP_ERROR_UNSUPPORTED;
    default:
        return BSP_ERROR_PARAM;
    }
}

static int32_t validate_spi_direction(const spi_instance_map_t &map, bool has_tx, bool has_rx)
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

static bool spi_dma_available(const spi_instance_map_t &map, const spi_dma_resource_t &dma, bool has_tx, bool has_rx)
{
    if (!spi_dma_resource_present(dma)) {
        return false;
    }
    if (has_tx && !spi_has_feature(map, BSP_SPI_FEATURE_DMA_TX)) {
        return false;
    }
    if (has_rx && !spi_has_feature(map, BSP_SPI_FEATURE_DMA_RX)) {
        return false;
    }
    return has_tx || has_rx;
}

static int32_t resolve_hpm_spi_async_mode(const spi_instance_map_t &map,
                                          const spi_dma_resource_t &dma,
                                          bool has_tx,
                                          bool has_rx,
                                          bsp_transfer_mode_t *out_mode)
{
    switch (map.transfer_mode) {
    case BSP_TRANSFER_MODE_AUTO:
        if (spi_dma_available(map, dma, has_tx, has_rx)) {
            *out_mode = BSP_TRANSFER_MODE_DMA;
            return BSP_OK;
        }
        if (spi_has_feature(map, BSP_SPI_FEATURE_INTERRUPT)) {
            *out_mode = BSP_TRANSFER_MODE_INTERRUPT;
            return BSP_OK;
        }
        return BSP_ERROR_UNSUPPORTED;
    case BSP_TRANSFER_MODE_DMA:
        if (spi_dma_available(map, dma, has_tx, has_rx)) {
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

static uint8_t to_dma_width(uint8_t data_len_in_bytes)
{
    switch (data_len_in_bytes) {
    case 1U:
        return DMA_TRANSFER_WIDTH_BYTE;
    case 2U:
        return DMA_TRANSFER_WIDTH_HALF_WORD;
    case 4U:
        return DMA_TRANSFER_WIDTH_WORD;
    case 8U:
        return DMA_TRANSFER_WIDTH_DOUBLE_WORD;
    default:
        return DMA_TRANSFER_WIDTH_BYTE;
    }
}

static void stop_spi_dma_transfer(spi_instance_map_t &map, const spi_dma_resource_t &dma)
{
    spi_disable_dma(map.base, spi_tx_dma_enable | spi_rx_dma_enable);

    dma_disable_channel_interrupt(dma.base, dma.tx_ch, DMA_INTERRUPT_MASK_ALL);
    dma_disable_channel_interrupt(dma.base, dma.rx_ch, DMA_INTERRUPT_MASK_ALL);
    dma_disable_channel(dma.base, dma.tx_ch);
    dma_disable_channel(dma.base, dma.rx_ch);

    map.dma_tx_active = false;
    map.dma_rx_active = false;
}

static void clear_all_dma_channel_status(DMA_Type *dma_base)
{
    for (uint32_t channel = 0U; channel < DMA_SOC_CHANNEL_NUM; ++channel) {
        dma_clear_transfer_status(dma_base, static_cast<uint8_t>(channel));
    }
}

static void finish_spi_async_transfer(spi_instance_map_t &map, int32_t status)
{
    map.async_in_flight = false;
    map.tx_remaining = 0U;
    map.rx_remaining = 0U;
    if (map.event_callback != nullptr) {
        bsp_spi_event_t event = BSP_SPI_EVENT_TRANSFER;
        if (map.tx_data != nullptr && map.rx_data == nullptr) {
            event = BSP_SPI_EVENT_WRITE;
        } else if (map.tx_data == nullptr && map.rx_data != nullptr) {
            event = BSP_SPI_EVENT_READ;
        }
        map.event_callback(event, status, map.event_callback_arg);
    }
}

static int32_t setup_spi_dma_channel(DMA_Type *dma_base,
                                     uint8_t channel,
                                     uint32_t source,
                                     uint32_t destination,
                                     bool source_fixed,
                                     bool destination_fixed,
                                     uint8_t data_width,
                                     uint32_t size_in_byte)
{
    dma_handshake_config_t config;
    dma_default_handshake_config(dma_base, &config);
    config.ch_index = channel;
    config.src = source;
    config.dst = destination;
    config.src_fixed = source_fixed;
    config.dst_fixed = destination_fixed;
    config.data_width = data_width;
    config.size_in_byte = size_in_byte;
    return (dma_setup_handshake(dma_base, &config, false) == status_success) ? BSP_OK : BSP_ERROR;
}

static void apply_spi_mode(spi_format_config_t *format_config, bsp_spi_mode_t mode)
{
    switch (mode) {
    case BSP_SPI_MODE_0:
        format_config->common_config.cpol = spi_sclk_low_idle;
        format_config->common_config.cpha = spi_sclk_sampling_odd_clk_edges;
        break;
    case BSP_SPI_MODE_1:
        format_config->common_config.cpol = spi_sclk_low_idle;
        format_config->common_config.cpha = spi_sclk_sampling_even_clk_edges;
        break;
    case BSP_SPI_MODE_2:
        format_config->common_config.cpol = spi_sclk_high_idle;
        format_config->common_config.cpha = spi_sclk_sampling_odd_clk_edges;
        break;
    case BSP_SPI_MODE_3:
    default:
        format_config->common_config.cpol = spi_sclk_high_idle;
        format_config->common_config.cpha = spi_sclk_sampling_even_clk_edges;
        break;
    }
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

    spi_instance_map_t *map = &s_spi_map[spi];
    spi_timing_config_t timing_config;
    spi_format_config_t format_config;
    uint32_t spi_clock;

    if (!map->has_pinmux) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (!spi_has_feature(*map, BSP_SPI_FEATURE_MASTER)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if ((frequency_hz != 0U) && !spi_has_feature(*map, BSP_SPI_FEATURE_CONFIGURABLE_CLOCK)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if ((bit_order == BSP_SPI_BIT_ORDER_MSB_FIRST) && !spi_has_feature(*map, BSP_SPI_FEATURE_MSB_FIRST)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if ((bit_order == BSP_SPI_BIT_ORDER_LSB_FIRST) && !spi_has_feature(*map, BSP_SPI_FEATURE_LSB_FIRST)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    const auto init_mode_status = validate_spi_init_mode(*map, s_spi_dma_map[spi], transfer_mode);
    if (init_mode_status != BSP_OK) {
        return init_mode_status;
    }

    spi_clock = board_init_spi_clock(map->base);
    board_init_spi_pins_with_gpio_as_cs(map->base);
    if (spi_clock == 0U) {
        return BSP_ERROR_UNSUPPORTED;
    }

    spi_master_get_default_timing_config(&timing_config);
    timing_config.master_config.clk_src_freq_in_hz = spi_clock;
    timing_config.master_config.sclk_freq_in_hz = (frequency_hz != 0U) ? frequency_hz : BOARD_APP_SPI_SCLK_FREQ;
    if (spi_master_timing_init(map->base, &timing_config) != status_success) {
        return BSP_ERROR;
    }

    spi_master_get_default_format_config(&format_config);
    format_config.common_config.data_len_in_bits = BOARD_APP_SPI_DATA_LEN_IN_BITS;
    format_config.common_config.mode = spi_master_mode;
    apply_spi_mode(&format_config, mode);
    format_config.common_config.lsb = (bit_order == BSP_SPI_BIT_ORDER_LSB_FIRST);
    spi_format_init(map->base, &format_config);

    map->transfer_mode = transfer_mode;
    map->irq_priority = normalize_irq_priority(irq_priority);
    map->async_in_flight = false;
    map->dma_tx_active = false;
    map->dma_rx_active = false;
    map->initialized = true;
    return BSP_OK;
}

int32_t bsp_spi_transfer(bsp_spi_instance_t spi, const uint8_t *tx_data, uint8_t *rx_data, uint32_t size)
{
    if ((spi >= BSP_SPI_MAX) || (size == 0U) || ((tx_data == nullptr) && (rx_data == nullptr))) {
        return BSP_ERROR_PARAM;
    }

    spi_instance_map_t *map = &s_spi_map[spi];
    spi_control_config_t control_config;

    if (!map->initialized) {
        int32_t status = bsp_spi_init(spi,
                                      BSP_SPI_MODE_3,
                                      BOARD_APP_SPI_SCLK_FREQ,
                                      BSP_SPI_BIT_ORDER_MSB_FIRST,
                                      BSP_TRANSFER_MODE_AUTO,
                                      0U);
        if (status != BSP_OK) {
            return status;
        }
    }
    const auto direction_status = validate_spi_direction(*map, tx_data != nullptr, rx_data != nullptr);
    if (direction_status != BSP_OK) {
        return direction_status;
    }
    if (!spi_has_feature(*map, BSP_SPI_FEATURE_BLOCKING)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    spi_master_get_default_control_config(&control_config);
    control_config.master_config.cmd_enable = false;
    control_config.master_config.addr_enable = false;
    if ((tx_data != nullptr) && (rx_data != nullptr)) {
        control_config.common_config.trans_mode = spi_trans_write_read_together;
    } else if (tx_data != nullptr) {
        control_config.common_config.trans_mode = spi_trans_write_only;
    } else {
        control_config.common_config.trans_mode = spi_trans_read_only;
    }

    if (spi_transfer(map->base, &control_config, nullptr, nullptr,
                     const_cast<uint8_t *>(tx_data), (tx_data != nullptr) ? size : 0U,
                     rx_data, (rx_data != nullptr) ? size : 0U) != status_success) {
        return BSP_ERROR_TIMEOUT;
    }

    return BSP_OK;
}

int32_t bsp_spi_transfer_async(bsp_spi_instance_t spi, const uint8_t *tx_data, uint8_t *rx_data, uint32_t size)
{
    if ((spi >= BSP_SPI_MAX) || (size == 0U) || ((tx_data == nullptr) && (rx_data == nullptr))) {
        return BSP_ERROR_PARAM;
    }
    auto &map = s_spi_map[spi];
    if (!map.initialized) {
        const auto status = bsp_spi_init(spi,
                                         BSP_SPI_MODE_3,
                                         BOARD_APP_SPI_SCLK_FREQ,
                                         BSP_SPI_BIT_ORDER_MSB_FIRST,
                                         BSP_TRANSFER_MODE_AUTO,
                                         0U);
        if (status != BSP_OK) {
            return status;
        }
    }
    const auto direction_status = validate_spi_direction(map, tx_data != nullptr, rx_data != nullptr);
    if (direction_status != BSP_OK) {
        return direction_status;
    }
    const auto &dma = s_spi_dma_map[spi];
    bsp_transfer_mode_t transfer_mode = BSP_TRANSFER_MODE_AUTO;
    const auto mode_status = resolve_hpm_spi_async_mode(map, dma, tx_data != nullptr, rx_data != nullptr, &transfer_mode);
    if (mode_status != BSP_OK) {
        return mode_status;
    }
    if (map.async_in_flight) {
        return BSP_ERROR_BUSY;
    }
    map.data_len_in_bytes = spi_get_data_length_in_bytes(map.base);
    if ((map.data_len_in_bytes == 0U) || ((size % map.data_len_in_bytes) != 0U)) {
        return BSP_ERROR_PARAM;
    }

    spi_master_get_default_control_config(&map.async_control);
    map.async_control.master_config.cmd_enable = false;
    map.async_control.master_config.addr_enable = false;
    map.async_control.common_config.tx_dma_enable = false;
    map.async_control.common_config.rx_dma_enable = false;
    if ((tx_data != nullptr) && (rx_data != nullptr)) {
        map.async_control.common_config.trans_mode = spi_trans_write_read_together;
    } else if (tx_data != nullptr) {
        map.async_control.common_config.trans_mode = spi_trans_write_only;
    } else {
        map.async_control.common_config.trans_mode = spi_trans_read_only;
    }

    map.tx_data = tx_data;
    map.rx_data = rx_data;
    map.tx_remaining = (tx_data != nullptr) ? size : 0U;
    map.rx_remaining = (rx_data != nullptr) ? size : 0U;
    map.async_in_flight = true;

    if (transfer_mode == BSP_TRANSFER_MODE_INTERRUPT) {
        if (spi_control_init(map.base,
                             &map.async_control,
                             map.tx_remaining / map.data_len_in_bytes,
                             map.rx_remaining / map.data_len_in_bytes) != status_success) {
            map.async_in_flight = false;
            map.tx_remaining = 0U;
            map.rx_remaining = 0U;
            return BSP_ERROR;
        }

        spi_set_tx_fifo_threshold(map.base, SPI_SOC_FIFO_DEPTH - 1U);
        spi_set_rx_fifo_threshold(map.base, 1U);
        spi_enable_interrupt(map.base, spi_tx_fifo_threshold_int | spi_rx_fifo_threshold_int | spi_end_int);
        intc_m_enable_irq_with_priority(map.irq, map.irq_priority);

        uint8_t cmd = 0U;
        if (spi_write_command(map.base, spi_master_mode, &map.async_control, &cmd) != status_success) {
            spi_disable_interrupt(map.base, spi_tx_fifo_threshold_int | spi_rx_fifo_threshold_int | spi_end_int);
            map.async_in_flight = false;
            map.tx_remaining = 0U;
            map.rx_remaining = 0U;
            return BSP_ERROR;
        }

        return BSP_OK;
    }

    const uint8_t dma_width = to_dma_width(map.data_len_in_bytes);
    const uint32_t tx_count = (tx_data != nullptr) ? (size / map.data_len_in_bytes) : 0U;
    const uint32_t rx_count = (rx_data != nullptr) ? (size / map.data_len_in_bytes) : 0U;

    spi_disable_dma(map.base, spi_tx_dma_enable | spi_rx_dma_enable);
    dma_clear_transfer_status(dma.base, dma.tx_ch);
    dma_clear_transfer_status(dma.base, dma.rx_ch);

    if (tx_data != nullptr) {
        dmamux_config(dma.dmamux, DMA_SOC_CHN_TO_DMAMUX_CHN(dma.base, dma.tx_ch), dma.tx_req, true);
        if (setup_spi_dma_channel(dma.base,
                                  dma.tx_ch,
                                  core_local_mem_to_sys_address(BOARD_RUNNING_CORE,
                                                                static_cast<uint32_t>(reinterpret_cast<uintptr_t>(tx_data))),
                                  static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&map.base->DATA)),
                                  false,
                                  true,
                                  dma_width,
                                  size) != BSP_OK) {
            stop_spi_dma_transfer(map, dma);
            map.async_in_flight = false;
            map.tx_remaining = 0U;
            map.rx_remaining = 0U;
            return BSP_ERROR;
        }
        map.dma_tx_active = true;
        map.async_control.common_config.tx_dma_enable = true;
    }

    if (rx_data != nullptr) {
        dmamux_config(dma.dmamux, DMA_SOC_CHN_TO_DMAMUX_CHN(dma.base, dma.rx_ch), dma.rx_req, true);
        if (setup_spi_dma_channel(dma.base,
                                  dma.rx_ch,
                                  static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&map.base->DATA)),
                                  core_local_mem_to_sys_address(BOARD_RUNNING_CORE,
                                                                static_cast<uint32_t>(reinterpret_cast<uintptr_t>(rx_data))),
                                  true,
                                  false,
                                  dma_width,
                                  size) != BSP_OK) {
            stop_spi_dma_transfer(map, dma);
            map.async_in_flight = false;
            map.tx_remaining = 0U;
            map.rx_remaining = 0U;
            return BSP_ERROR;
        }
        map.dma_rx_active = true;
        map.async_control.common_config.rx_dma_enable = true;
    }

    clear_all_dma_channel_status(dma.base);
    if (map.dma_tx_active) {
        dma_enable_channel_interrupt(dma.base,
                                     dma.tx_ch,
                                     DMA_INTERRUPT_MASK_TERMINAL_COUNT | DMA_INTERRUPT_MASK_ERROR | DMA_INTERRUPT_MASK_ABORT);
    }
    if (map.dma_rx_active) {
        dma_enable_channel_interrupt(dma.base,
                                     dma.rx_ch,
                                     DMA_INTERRUPT_MASK_TERMINAL_COUNT | DMA_INTERRUPT_MASK_ERROR | DMA_INTERRUPT_MASK_ABORT);
    }
    intc_m_enable_irq_with_priority(dma.irq, map.irq_priority);
    if (map.dma_rx_active && (dma_enable_channel(dma.base, dma.rx_ch) != status_success)) {
        stop_spi_dma_transfer(map, dma);
        map.async_in_flight = false;
        map.tx_remaining = 0U;
        map.rx_remaining = 0U;
        return BSP_ERROR;
    }
    if (map.dma_tx_active && (dma_enable_channel(dma.base, dma.tx_ch) != status_success)) {
        stop_spi_dma_transfer(map, dma);
        map.async_in_flight = false;
        map.tx_remaining = 0U;
        map.rx_remaining = 0U;
        return BSP_ERROR;
    }

    if (spi_control_init(map.base, &map.async_control, tx_count, rx_count) != status_success) {
        stop_spi_dma_transfer(map, dma);
        map.async_in_flight = false;
        map.tx_remaining = 0U;
        map.rx_remaining = 0U;
        return BSP_ERROR;
    }

    if (map.async_control.common_config.tx_dma_enable) {
#if defined(HPM_IP_FEATURE_SPI_DMA_TX_REQ_AFTER_CMD_FO_MASTER) && (HPM_IP_FEATURE_SPI_DMA_TX_REQ_AFTER_CMD_FO_MASTER == 1)
        map.base->CTRL |= SPI_CTRL_CMD_OP_MASK;
#endif
        map.base->CTRL |= SPI_CTRL_TXDMAEN_MASK;
    }
    if (map.async_control.common_config.rx_dma_enable) {
        map.base->CTRL |= SPI_CTRL_RXDMAEN_MASK;
    }
    if (spi_write_address(map.base, spi_master_mode, &map.async_control, nullptr) != status_success) {
        stop_spi_dma_transfer(map, dma);
        map.async_in_flight = false;
        map.tx_remaining = 0U;
        map.rx_remaining = 0U;
        return BSP_ERROR;
    }

    uint8_t cmd = 0U;
    if (spi_write_command(map.base, spi_master_mode, &map.async_control, &cmd) != status_success) {
        stop_spi_dma_transfer(map, dma);
        map.async_in_flight = false;
        map.tx_remaining = 0U;
        map.rx_remaining = 0U;
        return BSP_ERROR;
    }

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

extern "C" void bsp_spi_irq_handler(bsp_spi_instance_t spi)
{
    if (spi >= BSP_SPI_MAX) {
        return;
    }

    auto &map = s_spi_map[spi];
    const uint32_t irq_status = spi_get_interrupt_status(map.base);

    if ((irq_status & spi_rx_fifo_threshold_int) != 0U) {
        if (map.rx_remaining > 0U) {
            if (spi_read_data(map.base, map.data_len_in_bytes, map.rx_data, 1U) == status_success) {
                map.rx_data += map.data_len_in_bytes;
                map.rx_remaining -= map.data_len_in_bytes;
            }
        }
        spi_clear_interrupt_status(map.base, spi_rx_fifo_threshold_int);
        if (map.rx_remaining == 0U) {
            spi_disable_interrupt(map.base, spi_rx_fifo_threshold_int);
        }
    }

    if ((irq_status & spi_tx_fifo_threshold_int) != 0U) {
        if (map.tx_remaining > 0U) {
            if (spi_write_data(map.base, map.data_len_in_bytes, const_cast<uint8_t *>(map.tx_data), 1U) == status_success) {
                map.tx_data += map.data_len_in_bytes;
                map.tx_remaining -= map.data_len_in_bytes;
            }
        }
        spi_clear_interrupt_status(map.base, spi_tx_fifo_threshold_int);
        if (map.tx_remaining == 0U) {
            spi_disable_interrupt(map.base, spi_tx_fifo_threshold_int);
        }
    }

    if ((irq_status & spi_end_int) != 0U) {
        spi_disable_interrupt(map.base, spi_end_int | spi_tx_fifo_threshold_int | spi_rx_fifo_threshold_int);
        spi_clear_interrupt_status(map.base, spi_end_int);
        finish_spi_async_transfer(map, BSP_OK);
    }
}

extern "C" void bsp_spi_dma_irq_handler(bsp_spi_instance_t spi)
{
    if (spi >= BSP_SPI_MAX) {
        return;
    }

    auto &map = s_spi_map[spi];
    if (!map.async_in_flight || (!map.dma_tx_active && !map.dma_rx_active)) {
        return;
    }

    const auto &dma = s_spi_dma_map[spi];
    bool has_error = false;

    if (map.dma_tx_active) {
        const uint32_t status = dma_check_transfer_status(dma.base, dma.tx_ch);
        if ((status & (DMA_CHANNEL_STATUS_TC | DMA_CHANNEL_STATUS_ERROR | DMA_CHANNEL_STATUS_ABORT)) != 0U) {
            dma_clear_transfer_status(dma.base, dma.tx_ch);
        }
        if ((status & (DMA_CHANNEL_STATUS_ERROR | DMA_CHANNEL_STATUS_ABORT)) != 0U) {
            has_error = true;
        }
        if ((status & DMA_CHANNEL_STATUS_TC) != 0U) {
            map.dma_tx_active = false;
            dma_disable_channel_interrupt(dma.base, dma.tx_ch, DMA_INTERRUPT_MASK_ALL);
            dma_disable_channel(dma.base, dma.tx_ch);
        }
    }

    if (map.dma_rx_active) {
        const uint32_t status = dma_check_transfer_status(dma.base, dma.rx_ch);
        if ((status & (DMA_CHANNEL_STATUS_TC | DMA_CHANNEL_STATUS_ERROR | DMA_CHANNEL_STATUS_ABORT)) != 0U) {
            dma_clear_transfer_status(dma.base, dma.rx_ch);
        }
        if ((status & (DMA_CHANNEL_STATUS_ERROR | DMA_CHANNEL_STATUS_ABORT)) != 0U) {
            has_error = true;
        }
        if ((status & DMA_CHANNEL_STATUS_TC) != 0U) {
            map.dma_rx_active = false;
            dma_disable_channel_interrupt(dma.base, dma.rx_ch, DMA_INTERRUPT_MASK_ALL);
            dma_disable_channel(dma.base, dma.rx_ch);
        }
    }

    if (has_error) {
        stop_spi_dma_transfer(map, dma);
        finish_spi_async_transfer(map, BSP_ERROR);
        return;
    }

    if (!map.dma_tx_active && !map.dma_rx_active) {
        stop_spi_dma_transfer(map, dma);
        finish_spi_async_transfer(map, BSP_OK);
    }
}

#define BSP_SPI_ISR_ITEM(name, base_addr, irq_num, clk_name, has_pinmux, features) \
    SDK_DECLARE_EXT_ISR_M(irq_num, hpm_bsp_##name##_isr)                 \
    void hpm_bsp_##name##_isr(void)                                      \
    {                                                                    \
        bsp_spi_irq_handler(name);                                       \
    }

BSP_SPI_LIST(BSP_SPI_ISR_ITEM)

#undef BSP_SPI_ISR_ITEM

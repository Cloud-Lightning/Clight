/**
 * @file bsp_i2c_hpm.cpp
 * @brief HPM platform implementation for BSP I2C interface
 */

#include "bsp_i2c.h"
#include "board.h"

extern "C" {
#include "hpm_dmav2_drv.h"
#include "hpm_dmamux_drv.h"
#include "hpm_i2c_drv.h"
#include "hpm_interrupt.h"
void board_i2c_bus_clear(I2C_Type *ptr);
}

typedef struct {
    I2C_Type *base;
    uint32_t irq;
    uint32_t clock_name;
    bool has_pinmux;
    uint32_t features;
    bool initialized;
    const uint8_t *tx_data;
    uint8_t *rx_data;
    uint32_t size;
    uint32_t transferred;
    bool read_direction;
    bsp_transfer_mode_t transfer_mode;
    uint8_t irq_priority;
    bool async_in_flight;
    bool dma_active;
    bool dma_done;
    bool i2c_done;
    bsp_i2c_event_callback_t event_callback;
    void *event_callback_arg;
} i2c_instance_map_t;

#define BSP_I2C_MAP_ITEM(name, base_addr, irq_num, clk_name, has_pinmux, features) \
    { base_addr, irq_num, clk_name, (has_pinmux) != 0U, static_cast<uint32_t>(features), false, nullptr, nullptr, 0U, 0U, false, BSP_TRANSFER_MODE_AUTO, BSP_IRQ_PRIORITY_DEFAULT, false, false, false, false, nullptr, nullptr },
static i2c_instance_map_t s_i2c_map[BSP_I2C_MAX] = {
    BSP_I2C_LIST(BSP_I2C_MAP_ITEM)
};
#undef BSP_I2C_MAP_ITEM

typedef struct {
    DMA_Type *base;
    uint32_t irq;
    DMAMUX_Type *dmamux;
    uint32_t req;
    uint8_t ch;
} i2c_dma_resource_t;

#define BSP_I2C_DMA_MAP_ITEM(name, base_addr, irq_num, clk_name, has_pinmux, features) \
    { name##_DMA_CONTROLLER, name##_DMA_IRQ, name##_DMAMUX, name##_DMA_REQ, name##_DMA_CH },
static const i2c_dma_resource_t s_i2c_dma_map[BSP_I2C_MAX] = {
    BSP_I2C_LIST(BSP_I2C_DMA_MAP_ITEM)
};
#undef BSP_I2C_DMA_MAP_ITEM

static uint8_t normalize_irq_priority(uint8_t irq_priority)
{
    return (irq_priority == 0U) ? BSP_IRQ_PRIORITY_DEFAULT : irq_priority;
}

static bool i2c_has_feature(const i2c_instance_map_t &map, uint32_t feature)
{
    return (map.features & feature) != 0U;
}

static bool i2c_dma_resource_present(const i2c_dma_resource_t &dma)
{
    return (dma.base != nullptr) && (dma.dmamux != nullptr) && (dma.req != UINT32_MAX);
}

static int32_t validate_i2c_init_mode(const i2c_instance_map_t &map,
                                      const i2c_dma_resource_t &dma,
                                      bsp_transfer_mode_t transfer_mode)
{
    switch (transfer_mode) {
    case BSP_TRANSFER_MODE_AUTO:
        return BSP_OK;
    case BSP_TRANSFER_MODE_BLOCKING:
        return i2c_has_feature(map, BSP_I2C_FEATURE_BLOCKING) ? BSP_OK : BSP_ERROR_UNSUPPORTED;
    case BSP_TRANSFER_MODE_INTERRUPT:
        return i2c_has_feature(map, BSP_I2C_FEATURE_INTERRUPT) ? BSP_OK : BSP_ERROR_UNSUPPORTED;
    case BSP_TRANSFER_MODE_DMA:
        return (i2c_has_feature(map, BSP_I2C_FEATURE_DMA) && i2c_dma_resource_present(dma))
                   ? BSP_OK
                   : BSP_ERROR_UNSUPPORTED;
    default:
        return BSP_ERROR_PARAM;
    }
}

static bool i2c_dma_available(const i2c_instance_map_t &map, const i2c_dma_resource_t &dma)
{
    return i2c_has_feature(map, BSP_I2C_FEATURE_DMA) && i2c_dma_resource_present(dma);
}

static int32_t resolve_hpm_i2c_async_mode(const i2c_instance_map_t &map,
                                          const i2c_dma_resource_t &dma,
                                          bsp_transfer_mode_t *out_mode)
{
    switch (map.transfer_mode) {
    case BSP_TRANSFER_MODE_AUTO:
        if (i2c_dma_available(map, dma)) {
            *out_mode = BSP_TRANSFER_MODE_DMA;
            return BSP_OK;
        }
        if (i2c_has_feature(map, BSP_I2C_FEATURE_INTERRUPT)) {
            *out_mode = BSP_TRANSFER_MODE_INTERRUPT;
            return BSP_OK;
        }
        return BSP_ERROR_UNSUPPORTED;
    case BSP_TRANSFER_MODE_DMA:
        if (i2c_dma_available(map, dma)) {
            *out_mode = BSP_TRANSFER_MODE_DMA;
            return BSP_OK;
        }
        return BSP_ERROR_UNSUPPORTED;
    case BSP_TRANSFER_MODE_INTERRUPT:
        if (i2c_has_feature(map, BSP_I2C_FEATURE_INTERRUPT)) {
            *out_mode = BSP_TRANSFER_MODE_INTERRUPT;
            return BSP_OK;
        }
        return BSP_ERROR_UNSUPPORTED;
    default:
        return BSP_ERROR_UNSUPPORTED;
    }
}

static uint8_t to_hpm_i2c_mode(uint32_t frequency_hz)
{
    if (frequency_hz == 0U || frequency_hz <= 100000U) {
        return i2c_mode_normal;
    }
    if (frequency_hz <= 400000U) {
        return i2c_mode_fast;
    }
    return i2c_mode_fast_plus;
}

static void stop_i2c_dma_transfer(i2c_instance_map_t &map, const i2c_dma_resource_t &dma)
{
    i2c_dma_disable(map.base);
    dma_disable_channel_interrupt(dma.base, dma.ch, DMA_INTERRUPT_MASK_ALL);
    dma_disable_channel(dma.base, dma.ch);
    dma_clear_transfer_status(dma.base, dma.ch);
    map.dma_active = false;
}

static void finish_i2c_async_transfer(i2c_instance_map_t &map, int32_t status)
{
    i2c_disable_irq(map.base, I2C_EVENT_TRANSACTION_COMPLETE | I2C_EVENT_FIFO_FULL | I2C_EVENT_FIFO_EMPTY);
    i2c_dma_disable(map.base);
    map.async_in_flight = false;
    map.dma_active = false;
    map.dma_done = false;
    map.i2c_done = false;
    map.transferred = 0U;
    if (map.event_callback != nullptr) {
        map.event_callback(map.read_direction ? BSP_I2C_EVENT_READ : BSP_I2C_EVENT_WRITE, status, map.event_callback_arg);
    }
}

static void complete_i2c_dma_if_ready(i2c_instance_map_t &map)
{
    if (map.async_in_flight && map.dma_done && map.i2c_done) {
        finish_i2c_async_transfer(map, BSP_OK);
    }
}

static int32_t setup_i2c_dma_channel(const i2c_dma_resource_t &dma,
                                     uint32_t source,
                                     uint32_t destination,
                                     bool source_fixed,
                                     bool destination_fixed,
                                     uint32_t size)
{
    dma_handshake_config_t config;
    dma_default_handshake_config(dma.base, &config);
    config.ch_index = dma.ch;
    config.src = source;
    config.dst = destination;
    config.src_fixed = source_fixed;
    config.dst_fixed = destination_fixed;
    config.data_width = DMA_TRANSFER_WIDTH_BYTE;
    config.size_in_byte = size;
    return (dma_setup_handshake(dma.base, &config, false) == status_success) ? BSP_OK : BSP_ERROR;
}

int32_t bsp_i2c_init(bsp_i2c_instance_t i2c,
                     uint32_t frequency_hz,
                     bsp_state_t ten_bit_addressing,
                     bsp_transfer_mode_t transfer_mode,
                     uint8_t irq_priority)
{
    if (i2c >= BSP_I2C_MAX) {
        return BSP_ERROR_PARAM;
    }

    i2c_instance_map_t *map = &s_i2c_map[i2c];
    i2c_config_t config;
    uint32_t src_freq;

    if (!map->has_pinmux) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (!i2c_has_feature(*map, BSP_I2C_FEATURE_MASTER)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if ((frequency_hz != 0U) && !i2c_has_feature(*map, BSP_I2C_FEATURE_CONFIGURABLE_CLOCK)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if ((ten_bit_addressing == BSP_ENABLE) && !i2c_has_feature(*map, BSP_I2C_FEATURE_TEN_BIT_ADDRESS)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    const auto init_mode_status = validate_i2c_init_mode(*map, s_i2c_dma_map[i2c], transfer_mode);
    if (init_mode_status != BSP_OK) {
        return init_mode_status;
    }
    src_freq = board_init_i2c_clock(map->base);
    if (src_freq == 0U) {
        return BSP_ERROR_UNSUPPORTED;
    }

    init_i2c_pins(map->base);
    board_i2c_bus_clear(map->base);
    config.i2c_mode = to_hpm_i2c_mode(frequency_hz);
    config.is_10bit_addressing = (ten_bit_addressing == BSP_ENABLE);
    if (i2c_init_master(map->base, src_freq, &config) != status_success) {
        return BSP_ERROR;
    }

    map->transfer_mode = transfer_mode;
    map->irq_priority = normalize_irq_priority(irq_priority);
    map->async_in_flight = false;
    map->dma_active = false;
    map->dma_done = false;
    map->i2c_done = false;
    map->initialized = true;
    return BSP_OK;
}

int32_t bsp_i2c_write(bsp_i2c_instance_t i2c, uint16_t device_address, const uint8_t *data, uint32_t size)
{
    if ((i2c >= BSP_I2C_MAX) || (data == nullptr) || (size == 0U)) {
        return BSP_ERROR_PARAM;
    }

    if (!s_i2c_map[i2c].initialized) {
        int32_t status = bsp_i2c_init(i2c, 0U, BSP_DISABLE, BSP_TRANSFER_MODE_AUTO, 0U);
        if (status != BSP_OK) {
            return status;
        }
    }
    if (!i2c_has_feature(s_i2c_map[i2c], BSP_I2C_FEATURE_BLOCKING)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    if (i2c_master_write(s_i2c_map[i2c].base, device_address, const_cast<uint8_t *>(data), size) != status_success) {
        return BSP_ERROR_TIMEOUT;
    }

    return BSP_OK;
}

int32_t bsp_i2c_read(bsp_i2c_instance_t i2c, uint16_t device_address, uint8_t *data, uint32_t size)
{
    if ((i2c >= BSP_I2C_MAX) || (data == nullptr) || (size == 0U)) {
        return BSP_ERROR_PARAM;
    }

    if (!s_i2c_map[i2c].initialized) {
        int32_t status = bsp_i2c_init(i2c, 0U, BSP_DISABLE, BSP_TRANSFER_MODE_AUTO, 0U);
        if (status != BSP_OK) {
            return status;
        }
    }
    if (!i2c_has_feature(s_i2c_map[i2c], BSP_I2C_FEATURE_BLOCKING)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    if (i2c_master_read(s_i2c_map[i2c].base, device_address, data, size) != status_success) {
        return BSP_ERROR_TIMEOUT;
    }

    return BSP_OK;
}

int32_t bsp_i2c_write_async(bsp_i2c_instance_t i2c, uint16_t device_address, const uint8_t *data, uint32_t size)
{
    if ((i2c >= BSP_I2C_MAX) || (data == nullptr) || (size == 0U)) {
        return BSP_ERROR_PARAM;
    }
    if (!s_i2c_map[i2c].initialized) {
        const auto status = bsp_i2c_init(i2c, 0U, BSP_DISABLE, BSP_TRANSFER_MODE_AUTO, 0U);
        if (status != BSP_OK) {
            return status;
        }
    }

    auto &map = s_i2c_map[i2c];
    const auto &dma = s_i2c_dma_map[i2c];
    bsp_transfer_mode_t mode = BSP_TRANSFER_MODE_AUTO;
    const auto mode_status = resolve_hpm_i2c_async_mode(map, dma, &mode);
    if (mode_status != BSP_OK) {
        return mode_status;
    }
    if (map.async_in_flight) {
        return BSP_ERROR_BUSY;
    }
    map.tx_data = data;
    map.rx_data = nullptr;
    map.size = size;
    map.transferred = 0U;
    map.read_direction = false;
    map.async_in_flight = true;

    if (mode == BSP_TRANSFER_MODE_DMA) {
        map.dma_done = false;
        map.i2c_done = false;
        i2c_dma_disable(map.base);
        dma_disable_channel(dma.base, dma.ch);
        dma_clear_transfer_status(dma.base, dma.ch);
        dmamux_config(dma.dmamux,
                      DMA_SOC_CHN_TO_DMAMUX_CHN(dma.base, dma.ch),
                      dma.req,
                      true);
        if (setup_i2c_dma_channel(dma,
                                  core_local_mem_to_sys_address(BOARD_RUNNING_CORE,
                                                                static_cast<uint32_t>(reinterpret_cast<uintptr_t>(data))),
                                  static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&map.base->DATA)),
                                  false,
                                  true,
                                  size) != BSP_OK) {
            stop_i2c_dma_transfer(map, dma);
            map.async_in_flight = false;
            return BSP_ERROR;
        }
        dma_enable_channel_interrupt(dma.base,
                                     dma.ch,
                                     DMA_INTERRUPT_MASK_TERMINAL_COUNT | DMA_INTERRUPT_MASK_ERROR | DMA_INTERRUPT_MASK_ABORT);
        i2c_enable_irq(map.base, I2C_EVENT_TRANSACTION_COMPLETE);
        intc_m_enable_irq_with_priority(map.irq, map.irq_priority);
        intc_m_enable_irq_with_priority(dma.irq, map.irq_priority);
        map.dma_active = true;
        if (dma_enable_channel(dma.base, dma.ch) != status_success) {
            stop_i2c_dma_transfer(map, dma);
            map.async_in_flight = false;
            return BSP_ERROR;
        }
        if (i2c_master_start_dma_write(map.base, device_address, size) != status_success) {
            stop_i2c_dma_transfer(map, dma);
            map.async_in_flight = false;
            return BSP_ERROR;
        }
        return BSP_OK;
    }

    if (i2c_master_configure_transfer(map.base, device_address, size, false) != status_success) {
        map.async_in_flight = false;
        return BSP_ERROR;
    }
    i2c_enable_irq(map.base, I2C_EVENT_TRANSACTION_COMPLETE | I2C_EVENT_FIFO_EMPTY);
    intc_m_enable_irq_with_priority(map.irq, map.irq_priority);
    return BSP_OK;
}

int32_t bsp_i2c_read_async(bsp_i2c_instance_t i2c, uint16_t device_address, uint8_t *data, uint32_t size)
{
    if ((i2c >= BSP_I2C_MAX) || (data == nullptr) || (size == 0U)) {
        return BSP_ERROR_PARAM;
    }
    if (!s_i2c_map[i2c].initialized) {
        const auto status = bsp_i2c_init(i2c, 0U, BSP_DISABLE, BSP_TRANSFER_MODE_AUTO, 0U);
        if (status != BSP_OK) {
            return status;
        }
    }

    auto &map = s_i2c_map[i2c];
    const auto &dma = s_i2c_dma_map[i2c];
    bsp_transfer_mode_t mode = BSP_TRANSFER_MODE_AUTO;
    const auto mode_status = resolve_hpm_i2c_async_mode(map, dma, &mode);
    if (mode_status != BSP_OK) {
        return mode_status;
    }
    if (map.async_in_flight) {
        return BSP_ERROR_BUSY;
    }
    map.tx_data = nullptr;
    map.rx_data = data;
    map.size = size;
    map.transferred = 0U;
    map.read_direction = true;
    map.async_in_flight = true;

    if (mode == BSP_TRANSFER_MODE_DMA) {
        map.dma_done = false;
        map.i2c_done = false;
        i2c_dma_disable(map.base);
        dma_disable_channel(dma.base, dma.ch);
        dma_clear_transfer_status(dma.base, dma.ch);
        dmamux_config(dma.dmamux,
                      DMA_SOC_CHN_TO_DMAMUX_CHN(dma.base, dma.ch),
                      dma.req,
                      true);
        if (setup_i2c_dma_channel(dma,
                                  static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&map.base->DATA)),
                                  core_local_mem_to_sys_address(BOARD_RUNNING_CORE,
                                                                static_cast<uint32_t>(reinterpret_cast<uintptr_t>(data))),
                                  true,
                                  false,
                                  size) != BSP_OK) {
            stop_i2c_dma_transfer(map, dma);
            map.async_in_flight = false;
            return BSP_ERROR;
        }
        dma_enable_channel_interrupt(dma.base,
                                     dma.ch,
                                     DMA_INTERRUPT_MASK_TERMINAL_COUNT | DMA_INTERRUPT_MASK_ERROR | DMA_INTERRUPT_MASK_ABORT);
        i2c_enable_irq(map.base, I2C_EVENT_TRANSACTION_COMPLETE);
        intc_m_enable_irq_with_priority(map.irq, map.irq_priority);
        intc_m_enable_irq_with_priority(dma.irq, map.irq_priority);
        map.dma_active = true;
        if (dma_enable_channel(dma.base, dma.ch) != status_success) {
            stop_i2c_dma_transfer(map, dma);
            map.async_in_flight = false;
            return BSP_ERROR;
        }
        if (i2c_master_start_dma_read(map.base, device_address, size) != status_success) {
            stop_i2c_dma_transfer(map, dma);
            map.async_in_flight = false;
            return BSP_ERROR;
        }
        return BSP_OK;
    }

    if (i2c_master_configure_transfer(map.base, device_address, size, true) != status_success) {
        map.async_in_flight = false;
        return BSP_ERROR;
    }
    i2c_enable_irq(map.base, I2C_EVENT_TRANSACTION_COMPLETE | I2C_EVENT_FIFO_FULL);
    intc_m_enable_irq_with_priority(map.irq, map.irq_priority);
    return BSP_OK;
}

int32_t bsp_i2c_register_event_callback(bsp_i2c_instance_t i2c, bsp_i2c_event_callback_t callback, void *arg)
{
    if (i2c >= BSP_I2C_MAX) {
        return BSP_ERROR_PARAM;
    }

    s_i2c_map[i2c].event_callback = callback;
    s_i2c_map[i2c].event_callback_arg = arg;
    return BSP_OK;
}

extern "C" void bsp_i2c_irq_handler(bsp_i2c_instance_t i2c)
{
    if (i2c >= BSP_I2C_MAX) {
        return;
    }

    auto &map = s_i2c_map[i2c];
    const uint32_t status = i2c_get_status(map.base);
    const uint32_t irq = i2c_get_irq_setting(map.base);

    if (map.async_in_flight && map.dma_active) {
        if ((status & I2C_EVENT_TRANSACTION_COMPLETE) != 0U) {
            i2c_clear_status(map.base, I2C_EVENT_TRANSACTION_COMPLETE);
            map.i2c_done = true;
            complete_i2c_dma_if_ready(map);
        }
        return;
    }

    if ((status & I2C_EVENT_FIFO_EMPTY) && (irq & I2C_EVENT_FIFO_EMPTY) && !map.read_direction) {
        while (!i2c_fifo_is_full(map.base) && (map.transferred < map.size)) {
            i2c_write_byte(map.base, map.tx_data[map.transferred++]);
            if (map.transferred == map.size) {
                i2c_disable_irq(map.base, I2C_EVENT_FIFO_EMPTY);
                break;
            }
        }
    }

    if ((status & I2C_EVENT_FIFO_FULL) && (irq & I2C_EVENT_FIFO_FULL) && map.read_direction) {
        while (!i2c_fifo_is_empty(map.base) && (map.transferred < map.size)) {
            map.rx_data[map.transferred++] = i2c_read_byte(map.base);
        }
        if (map.transferred == map.size) {
            i2c_disable_irq(map.base, I2C_EVENT_FIFO_FULL);
        }
    }

    if (status & I2C_EVENT_TRANSACTION_COMPLETE) {
        i2c_disable_irq(map.base, I2C_EVENT_TRANSACTION_COMPLETE | I2C_EVENT_FIFO_FULL | I2C_EVENT_FIFO_EMPTY);
        i2c_clear_status(map.base, I2C_EVENT_TRANSACTION_COMPLETE);
        finish_i2c_async_transfer(map, BSP_OK);
    }
}

extern "C" void bsp_i2c_dma_irq_handler(bsp_i2c_instance_t i2c)
{
    if (i2c >= BSP_I2C_MAX) {
        return;
    }

    auto &map = s_i2c_map[i2c];
    const auto &dma = s_i2c_dma_map[i2c];
    if (!map.async_in_flight || !map.dma_active) {
        return;
    }

    const uint32_t status = dma_check_transfer_status(dma.base, dma.ch);
    if ((status & (DMA_CHANNEL_STATUS_TC | DMA_CHANNEL_STATUS_ERROR | DMA_CHANNEL_STATUS_ABORT)) != 0U) {
        dma_clear_transfer_status(dma.base, dma.ch);
    }
    if ((status & (DMA_CHANNEL_STATUS_ERROR | DMA_CHANNEL_STATUS_ABORT)) != 0U) {
        stop_i2c_dma_transfer(map, dma);
        finish_i2c_async_transfer(map, BSP_ERROR);
        return;
    }
    if ((status & DMA_CHANNEL_STATUS_TC) != 0U) {
        dma_disable_channel_interrupt(dma.base, dma.ch, DMA_INTERRUPT_MASK_ALL);
        dma_disable_channel(dma.base, dma.ch);
        i2c_dma_disable(map.base);
        map.dma_active = false;
        map.dma_done = true;
        if ((i2c_get_status(map.base) & I2C_EVENT_TRANSACTION_COMPLETE) != 0U) {
            i2c_clear_status(map.base, I2C_EVENT_TRANSACTION_COMPLETE);
            map.i2c_done = true;
        }
        complete_i2c_dma_if_ready(map);
    }
}

#define BSP_I2C_ISR_ITEM(name, base_addr, irq_num, clk_name, has_pinmux, features) \
    SDK_DECLARE_EXT_ISR_M(irq_num, hpm_bsp_##name##_isr)                 \
    void hpm_bsp_##name##_isr(void)                                      \
    {                                                                    \
        bsp_i2c_irq_handler(name);                                       \
    }

BSP_I2C_LIST(BSP_I2C_ISR_ITEM)

#undef BSP_I2C_ISR_ITEM

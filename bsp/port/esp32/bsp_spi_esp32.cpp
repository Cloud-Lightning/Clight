#include "bsp_spi.h"

#include "driver/spi_master.h"

typedef struct {
    spi_host_device_t host;
    int sclk_pin;
    int mosi_pin;
    int miso_pin;
    int clock_hz;
    bool initialized;
    spi_device_handle_t device;
    spi_transaction_t async_transaction;
    bool async_in_flight;
    bsp_spi_event_callback_t event_callback;
    void *event_callback_arg;
    bsp_spi_event_t pending_event;
} spi_instance_map_t;

#define BSP_SPI_MAP_ITEM(name, base, irq, clk, has_pinmux, features) \
    { static_cast<spi_host_device_t>(base), static_cast<int>(name##_SCLK_PIN), static_cast<int>(name##_MOSI_PIN), static_cast<int>(name##_MISO_PIN), static_cast<int>(name##_CLOCK_HZ), false, nullptr, {}, false, nullptr, nullptr, BSP_SPI_EVENT_TRANSFER },
static spi_instance_map_t s_spi_map[BSP_SPI_MAX] = {
    BSP_SPI_LIST(BSP_SPI_MAP_ITEM)
};
#undef BSP_SPI_MAP_ITEM

static void IRAM_ATTR spi_post_callback(spi_transaction_t *trans)
{
    auto *map = static_cast<spi_instance_map_t *>(trans->user);
    if (map != nullptr) {
        map->async_in_flight = false;
        if (map->event_callback != nullptr) {
            map->event_callback(map->pending_event, BSP_OK, map->event_callback_arg);
        }
    }
}

int32_t bsp_spi_init(bsp_spi_instance_t spi,
                     bsp_spi_mode_t mode,
                     uint32_t frequency_hz,
                     bsp_spi_bit_order_t bit_order,
                     bsp_transfer_mode_t transfer_mode,
                     uint8_t irq_priority)
{
    (void) transfer_mode;
    (void) irq_priority;
    if (spi >= BSP_SPI_MAX) {
        return BSP_ERROR_PARAM;
    }

    auto *map = &s_spi_map[spi];
    if (map->initialized) {
        return BSP_OK;
    }

    spi_bus_config_t bus_config = {};
    bus_config.sclk_io_num = map->sclk_pin;
    bus_config.mosi_io_num = map->mosi_pin;
    bus_config.miso_io_num = map->miso_pin;
    bus_config.quadwp_io_num = -1;
    bus_config.quadhd_io_num = -1;

    const auto bus_status = spi_bus_initialize(map->host, &bus_config, SPI_DMA_CH_AUTO);
    if (bus_status != ESP_OK && bus_status != ESP_ERR_INVALID_STATE) {
        return BSP_ERROR;
    }

    spi_device_interface_config_t device_config = {};
    device_config.clock_speed_hz = (frequency_hz != 0U) ? static_cast<int>(frequency_hz) : map->clock_hz;
    device_config.mode = static_cast<uint8_t>(mode);
    device_config.spics_io_num = -1;
    device_config.queue_size = 1;
    device_config.post_cb = spi_post_callback;
    device_config.flags = (bit_order == BSP_SPI_BIT_ORDER_LSB_FIRST) ? SPI_DEVICE_BIT_LSBFIRST : 0;

    if (spi_bus_add_device(map->host, &device_config, &map->device) != ESP_OK) {
        return BSP_ERROR;
    }

    map->initialized = true;
    return BSP_OK;
}

int32_t bsp_spi_transfer(bsp_spi_instance_t spi, const uint8_t *tx_data, uint8_t *rx_data, uint32_t size)
{
    if ((spi >= BSP_SPI_MAX) || (size == 0U) || ((tx_data == nullptr) && (rx_data == nullptr))) {
        return BSP_ERROR_PARAM;
    }
    if (bsp_spi_init(spi, BSP_SPI_MODE_3, 0U, BSP_SPI_BIT_ORDER_MSB_FIRST, BSP_TRANSFER_MODE_AUTO, 0U) != BSP_OK) {
        return BSP_ERROR;
    }

    spi_transaction_t transaction = {};
    transaction.length = size * 8U;
    transaction.tx_buffer = tx_data;
    transaction.rx_buffer = rx_data;

    return (spi_device_transmit(s_spi_map[spi].device, &transaction) == ESP_OK) ? BSP_OK : BSP_ERROR_TIMEOUT;
}

int32_t bsp_spi_transfer_async(bsp_spi_instance_t spi, const uint8_t *tx_data, uint8_t *rx_data, uint32_t size)
{
    if ((spi >= BSP_SPI_MAX) || (size == 0U) || ((tx_data == nullptr) && (rx_data == nullptr))) {
        return BSP_ERROR_PARAM;
    }
    if (bsp_spi_init(spi, BSP_SPI_MODE_3, 0U, BSP_SPI_BIT_ORDER_MSB_FIRST, BSP_TRANSFER_MODE_AUTO, 0U) != BSP_OK) {
        return BSP_ERROR;
    }

    auto *map = &s_spi_map[spi];
    if (map->async_in_flight) {
        return BSP_ERROR_BUSY;
    }

    map->async_transaction = {};
    map->async_transaction.length = size * 8U;
    map->async_transaction.tx_buffer = tx_data;
    map->async_transaction.rx_buffer = rx_data;
    map->async_transaction.user = map;
    map->pending_event = (tx_data != nullptr && rx_data != nullptr) ? BSP_SPI_EVENT_TRANSFER :
                         (tx_data != nullptr ? BSP_SPI_EVENT_WRITE : BSP_SPI_EVENT_READ);

    if (spi_device_queue_trans(map->device, &map->async_transaction, 0) != ESP_OK) {
        return BSP_ERROR_BUSY;
    }

    map->async_in_flight = true;
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

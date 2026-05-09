#include "bsp_i2c.h"

#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"

typedef struct {
    i2c_port_t port;
    int scl_pin;
    int sda_pin;
    uint32_t clock_hz;
    bool initialized;
    bsp_i2c_event_callback_t event_callback;
    void *event_callback_arg;
} i2c_instance_map_t;

#define BSP_I2C_MAP_ITEM(name, base, irq, clk, has_pinmux, features) \
    { static_cast<i2c_port_t>(base), static_cast<int>(name##_SCL_PIN), static_cast<int>(name##_SDA_PIN), static_cast<uint32_t>(name##_CLOCK_HZ), false, nullptr, nullptr },
static i2c_instance_map_t s_i2c_map[BSP_I2C_MAX] = {
    BSP_I2C_LIST(BSP_I2C_MAP_ITEM)
};
#undef BSP_I2C_MAP_ITEM

int32_t bsp_i2c_init(bsp_i2c_instance_t i2c,
                     uint32_t frequency_hz,
                     bsp_state_t ten_bit_addressing,
                     bsp_transfer_mode_t transfer_mode,
                     uint8_t irq_priority)
{
    (void) transfer_mode;
    (void) irq_priority;
    if (i2c >= BSP_I2C_MAX) {
        return BSP_ERROR_PARAM;
    }

    auto *map = &s_i2c_map[i2c];
    if (map->initialized) {
        return BSP_OK;
    }

    i2c_config_t config = {};
    config.mode = I2C_MODE_MASTER;
    config.scl_io_num = static_cast<gpio_num_t>(map->scl_pin);
    config.sda_io_num = static_cast<gpio_num_t>(map->sda_pin);
    config.scl_pullup_en = GPIO_PULLUP_ENABLE;
    config.sda_pullup_en = GPIO_PULLUP_ENABLE;
    if (ten_bit_addressing == BSP_ENABLE) {
        return BSP_ERROR_UNSUPPORTED;
    }
    config.master.clk_speed = (frequency_hz != 0U) ? frequency_hz : map->clock_hz;

    if (i2c_param_config(map->port, &config) != ESP_OK) {
        return BSP_ERROR;
    }
    const auto install_status = i2c_driver_install(map->port, config.mode, 0, 0, 0);
    if (install_status != ESP_OK && install_status != ESP_ERR_INVALID_STATE) {
        return BSP_ERROR;
    }

    map->initialized = true;
    return BSP_OK;
}

int32_t bsp_i2c_write(bsp_i2c_instance_t i2c, uint16_t device_address, const uint8_t *data, uint32_t size)
{
    if ((i2c >= BSP_I2C_MAX) || (data == nullptr) || (size == 0U)) {
        return BSP_ERROR_PARAM;
    }
    if (bsp_i2c_init(i2c, 0U, BSP_DISABLE, BSP_TRANSFER_MODE_AUTO, 0U) != BSP_OK) {
        return BSP_ERROR;
    }

    return (i2c_master_write_to_device(s_i2c_map[i2c].port,
                                       device_address,
                                       data,
                                       size,
                                       pdMS_TO_TICKS(100)) == ESP_OK) ? BSP_OK : BSP_ERROR_TIMEOUT;
}

int32_t bsp_i2c_read(bsp_i2c_instance_t i2c, uint16_t device_address, uint8_t *data, uint32_t size)
{
    if ((i2c >= BSP_I2C_MAX) || (data == nullptr) || (size == 0U)) {
        return BSP_ERROR_PARAM;
    }
    if (bsp_i2c_init(i2c, 0U, BSP_DISABLE, BSP_TRANSFER_MODE_AUTO, 0U) != BSP_OK) {
        return BSP_ERROR;
    }

    return (i2c_master_read_from_device(s_i2c_map[i2c].port,
                                        device_address,
                                        data,
                                        size,
                                        pdMS_TO_TICKS(100)) == ESP_OK) ? BSP_OK : BSP_ERROR_TIMEOUT;
}

int32_t bsp_i2c_write_async(bsp_i2c_instance_t i2c, uint16_t device_address, const uint8_t *data, uint32_t size)
{
    if (i2c >= BSP_I2C_MAX) {
        return BSP_ERROR_PARAM;
    }
    auto &map = s_i2c_map[i2c];
    if (map.event_callback == nullptr) {
        return BSP_ERROR_PARAM;
    }
    const auto status = bsp_i2c_write(i2c, device_address, data, size);
    if (status == BSP_OK) {
        map.event_callback(BSP_I2C_EVENT_WRITE, BSP_OK, map.event_callback_arg);
    }
    return status;
}

int32_t bsp_i2c_read_async(bsp_i2c_instance_t i2c, uint16_t device_address, uint8_t *data, uint32_t size)
{
    if (i2c >= BSP_I2C_MAX) {
        return BSP_ERROR_PARAM;
    }
    auto &map = s_i2c_map[i2c];
    if (map.event_callback == nullptr) {
        return BSP_ERROR_PARAM;
    }
    const auto status = bsp_i2c_read(i2c, device_address, data, size);
    if (status == BSP_OK) {
        map.event_callback(BSP_I2C_EVENT_READ, BSP_OK, map.event_callback_arg);
    }
    return status;
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

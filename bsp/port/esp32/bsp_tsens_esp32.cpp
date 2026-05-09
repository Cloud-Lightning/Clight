#include "bsp_tsens.h"

extern "C" {
#include "driver/temperature_sensor.h"
}

typedef struct {
    temperature_sensor_handle_t handle;
    bool initialized;
} tsens_map_t;

static tsens_map_t s_tsens_map[BSP_TSENS_MAX] = {};

int32_t bsp_tsens_init(bsp_tsens_instance_t instance)
{
    if (instance >= BSP_TSENS_MAX) {
        return BSP_ERROR_PARAM;
    }

    auto &map = s_tsens_map[instance];
    if (map.initialized) {
        return BSP_OK;
    }

    temperature_sensor_config_t config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(10, 50);
    if (temperature_sensor_install(&config, &map.handle) != ESP_OK) {
        return BSP_ERROR;
    }
    if (temperature_sensor_enable(map.handle) != ESP_OK) {
        return BSP_ERROR;
    }

    map.initialized = true;
    return BSP_OK;
}

int32_t bsp_tsens_read_celsius(bsp_tsens_instance_t instance, float *celsius)
{
    if ((instance >= BSP_TSENS_MAX) || (celsius == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    const auto init_status = bsp_tsens_init(instance);
    if (init_status != BSP_OK) {
        return init_status;
    }

    return (temperature_sensor_get_celsius(s_tsens_map[instance].handle, celsius) == ESP_OK) ? BSP_OK : BSP_ERROR;
}

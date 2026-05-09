#include "bsp_i2s.h"

typedef struct {
    uintptr_t base;
    bool available;
    uint32_t features;
} i2s_map_t;

#define BSP_I2S_MAP_ITEM(name, base, available, features) { (uintptr_t)(base), (available) != 0U, features },
static const i2s_map_t s_i2s_map[BSP_I2S_MAX] = {
    BSP_I2S_LIST(BSP_I2S_MAP_ITEM)
};
#undef BSP_I2S_MAP_ITEM

static int32_t i2s_require(bsp_i2s_instance_t instance, uint32_t feature)
{
    if (instance >= BSP_I2S_MAX) {
        return BSP_ERROR_PARAM;
    }
    const i2s_map_t *map = &s_i2s_map[instance];
    return (map->available && ((map->features & feature) != 0U)) ? BSP_OK : BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_i2s_init(bsp_i2s_instance_t instance)
{
    return i2s_require(instance, BSP_I2S_FEATURE_INIT);
}

int32_t bsp_i2s_write(bsp_i2s_instance_t instance, const uint8_t *data, uint32_t len)
{
    if ((data == nullptr) || (len == 0U)) {
        return BSP_ERROR_PARAM;
    }
    return i2s_require(instance, BSP_I2S_FEATURE_BLOCKING_WRITE);
}

int32_t bsp_i2s_read(bsp_i2s_instance_t instance, uint8_t *data, uint32_t len)
{
    if ((data == nullptr) || (len == 0U)) {
        return BSP_ERROR_PARAM;
    }
    return i2s_require(instance, BSP_I2S_FEATURE_BLOCKING_READ);
}

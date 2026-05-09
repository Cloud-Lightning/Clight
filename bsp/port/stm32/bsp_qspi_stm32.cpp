#include "bsp_qspi.h"

typedef struct {
    uintptr_t base;
    bool available;
    uint32_t features;
} qspi_map_t;

#define BSP_QSPI_MAP_ITEM(name, base, available, features) { (uintptr_t)(base), (available) != 0U, features },
static const qspi_map_t s_qspi_map[BSP_QSPI_MAX] = {
    BSP_QSPI_LIST(BSP_QSPI_MAP_ITEM)
};
#undef BSP_QSPI_MAP_ITEM

static int32_t qspi_require(bsp_qspi_instance_t instance, uint32_t feature)
{
    if (instance >= BSP_QSPI_MAX) {
        return BSP_ERROR_PARAM;
    }
    const qspi_map_t *map = &s_qspi_map[instance];
    return (map->available && ((map->features & feature) != 0U)) ? BSP_OK : BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_qspi_init(bsp_qspi_instance_t instance)
{
    return qspi_require(instance, BSP_QSPI_FEATURE_INIT);
}

int32_t bsp_qspi_read(bsp_qspi_instance_t instance, uint32_t address, uint8_t *data, uint32_t len)
{
    (void) address;
    if ((data == nullptr) || (len == 0U)) {
        return BSP_ERROR_PARAM;
    }
    return qspi_require(instance, BSP_QSPI_FEATURE_MEM_READ);
}

int32_t bsp_qspi_write(bsp_qspi_instance_t instance, uint32_t address, const uint8_t *data, uint32_t len)
{
    (void) address;
    if ((data == nullptr) || (len == 0U)) {
        return BSP_ERROR_PARAM;
    }
    return qspi_require(instance, BSP_QSPI_FEATURE_MEM_WRITE);
}

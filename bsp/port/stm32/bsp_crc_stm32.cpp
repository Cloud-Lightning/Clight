#include "bsp_crc.h"

typedef struct {
    CRC_TypeDef *base;
    uint32_t clock_name;
    uint32_t features;
} crc_map_t;

#define BSP_CRC_MAP_ITEM(name, base, clk, features) { base, clk, features },
static const crc_map_t s_crc_map[BSP_CRC_MAX] = {
    BSP_CRC_LIST(BSP_CRC_MAP_ITEM)
};
#undef BSP_CRC_MAP_ITEM

static uint32_t crc32_software(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFUL;
    for (uint32_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (uint32_t bit = 0; bit < 8U; ++bit) {
            const uint32_t mask = 0U - (crc & 1U);
            crc = (crc >> 1U) ^ (0xEDB88320UL & mask);
        }
    }
    return crc ^ 0xFFFFFFFFUL;
}

int32_t bsp_crc32_calculate(bsp_crc_instance_t instance, const uint8_t *data, uint32_t len, uint32_t *crc_out)
{
    if ((instance >= BSP_CRC_MAX) || (data == nullptr) || (len == 0U) || (crc_out == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    const crc_map_t *map = &s_crc_map[instance];
    if ((map->features & BSP_CRC_FEATURE_CRC32) == 0U) {
        return BSP_ERROR_UNSUPPORTED;
    }

#ifdef __HAL_RCC_CRC_CLK_ENABLE
    if (map->base == CRC) {
        __HAL_RCC_CRC_CLK_ENABLE();
    }
#endif

    *crc_out = crc32_software(data, len);
    return BSP_OK;
}

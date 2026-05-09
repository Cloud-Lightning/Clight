#include "bsp_dac.h"

typedef struct {
    DAC_TypeDef *base;
    bool has_pinmux;
    uint32_t features;
} dac_map_t;

#define BSP_DAC_MAP_ITEM(name, base, has_pinmux, features) { base, (has_pinmux) != 0U, features },
static const dac_map_t s_dac_map[BSP_DAC_MAX] = {
    BSP_DAC_LIST(BSP_DAC_MAP_ITEM)
};
#undef BSP_DAC_MAP_ITEM

static int32_t dac_status_for_board(bsp_dac_instance_t instance, uint32_t required_feature)
{
    if (instance >= BSP_DAC_MAX) {
        return BSP_ERROR_PARAM;
    }
    const dac_map_t *map = &s_dac_map[instance];
    if (!map->has_pinmux || (map->base == nullptr) || ((map->features & required_feature) == 0U)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    return BSP_OK;
}

int32_t bsp_dac_init(bsp_dac_instance_t instance)
{
    const int32_t status = dac_status_for_board(instance, BSP_DAC_FEATURE_DIRECT_WRITE);
    if (status != BSP_OK) {
        return status;
    }
#ifdef __HAL_RCC_DAC12_CLK_ENABLE
    if (s_dac_map[instance].base == DAC1) {
        __HAL_RCC_DAC12_CLK_ENABLE();
    }
#endif
    return BSP_OK;
}

int32_t bsp_dac_write(bsp_dac_instance_t instance, uint8_t channel, uint32_t value)
{
    if ((channel == 0U) || (channel > 2U) || (value > 4095U)) {
        return BSP_ERROR_PARAM;
    }
    const int32_t status = dac_status_for_board(instance, BSP_DAC_FEATURE_DIRECT_WRITE);
    if (status != BSP_OK) {
        return status;
    }
    DAC_TypeDef *base = s_dac_map[instance].base;
    if (channel == 1U) {
        base->CR |= DAC_CR_EN1;
        base->DHR12R1 = value & DAC_DHR12R1_DACC1DHR;
    } else {
#ifdef DAC_CR_EN2
        base->CR |= DAC_CR_EN2;
        base->DHR12R2 = value & DAC_DHR12R2_DACC2DHR;
#else
        return BSP_ERROR_UNSUPPORTED;
#endif
    }
    return BSP_OK;
}

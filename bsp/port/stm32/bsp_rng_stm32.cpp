#include "bsp_rng.h"

typedef struct {
    RNG_TypeDef *base;
    uint32_t features;
    bool initialized;
} rng_map_t;

#define BSP_RNG_MAP_ITEM(name, base, features) { base, features, false },
static rng_map_t s_rng_map[BSP_RNG_MAX] = {
    BSP_RNG_LIST(BSP_RNG_MAP_ITEM)
};
#undef BSP_RNG_MAP_ITEM

static bool rng_has_feature(bsp_rng_instance_t instance, uint32_t feature)
{
    return (instance < BSP_RNG_MAX) && ((s_rng_map[instance].features & feature) != 0U);
}

int32_t bsp_rng_init(bsp_rng_instance_t instance)
{
    if (instance >= BSP_RNG_MAX) {
        return BSP_ERROR_PARAM;
    }
    if (!rng_has_feature(instance, BSP_RNG_FEATURE_U32) || (s_rng_map[instance].base == nullptr)) {
        return BSP_ERROR_UNSUPPORTED;
    }

#ifdef __HAL_RCC_RNG_CLK_ENABLE
    __HAL_RCC_RNG_CLK_ENABLE();
#endif
    s_rng_map[instance].base->CR |= RNG_CR_RNGEN;
    s_rng_map[instance].initialized = true;
    return BSP_OK;
}

int32_t bsp_rng_get_u32(bsp_rng_instance_t instance, uint32_t *value)
{
    if ((instance >= BSP_RNG_MAX) || (value == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    if (!rng_has_feature(instance, BSP_RNG_FEATURE_U32) || (s_rng_map[instance].base == nullptr)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (!s_rng_map[instance].initialized) {
        const int32_t status = bsp_rng_init(instance);
        if (status != BSP_OK) {
            return status;
        }
    }

    RNG_TypeDef *base = s_rng_map[instance].base;
    for (uint32_t timeout = 0U; timeout < 1000000UL; ++timeout) {
        const uint32_t status = base->SR;
        if ((status & (RNG_SR_SEIS | RNG_SR_CEIS)) != 0U) {
            base->SR = 0U;
            return BSP_ERROR;
        }
        if ((status & RNG_SR_DRDY) != 0U) {
            *value = base->DR;
            return BSP_OK;
        }
    }

    return BSP_ERROR_TIMEOUT;
}

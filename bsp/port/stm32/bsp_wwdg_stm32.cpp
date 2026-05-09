#include "bsp_wwdg.h"

typedef struct {
    WWDG_TypeDef *base;
    IRQn_Type irq;
    uint32_t features;
    bool initialized;
} wwdg_map_t;

#define BSP_WWDG_MAP_ITEM(name, base, irq, features) { base, irq, features, false },
static wwdg_map_t s_wwdg_map[BSP_WWDG_MAX] = {
    BSP_WWDG_LIST(BSP_WWDG_MAP_ITEM)
};
#undef BSP_WWDG_MAP_ITEM

static uint32_t wwdg_pclk_hz(void)
{
#ifdef HAL_RCC_MODULE_ENABLED
    const uint32_t pclk = HAL_RCC_GetPCLK1Freq();
    return (pclk != 0U) ? pclk : 64000000U;
#else
    return 64000000U;
#endif
}

static int32_t wwdg_pick_timing(uint32_t timeout_ms, uint32_t *wdgtb, uint32_t *counter)
{
    const uint32_t pclk = wwdg_pclk_hz();
    for (uint32_t prescaler = 0U; prescaler <= 7U; ++prescaler) {
        const uint64_t tick_us = (4096ULL * 1000000ULL * (1ULL << prescaler)) / pclk;
        if (tick_us == 0ULL) {
            continue;
        }
        const uint64_t steps = (static_cast<uint64_t>(timeout_ms) * 1000ULL + tick_us - 1ULL) / tick_us;
        if ((steps > 0ULL) && (steps <= 64ULL)) {
            *wdgtb = prescaler;
            *counter = 0x3FUL + static_cast<uint32_t>(steps);
            if (*counter > 0x7FU) {
                *counter = 0x7FU;
            }
            return BSP_OK;
        }
    }
    return BSP_ERROR_PARAM;
}

int32_t bsp_wwdg_init(bsp_wwdg_instance_t instance, uint32_t timeout_ms)
{
    if ((instance >= BSP_WWDG_MAX) || (timeout_ms == 0U)) {
        return BSP_ERROR_PARAM;
    }
    wwdg_map_t *map = &s_wwdg_map[instance];
    if (((map->features & BSP_WWDG_FEATURE_INIT) == 0U) || (map->base == nullptr)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    uint32_t wdgtb = 0U;
    uint32_t counter = 0U;
    const int32_t timing_status = wwdg_pick_timing(timeout_ms, &wdgtb, &counter);
    if (timing_status != BSP_OK) {
        return timing_status;
    }

#ifdef __HAL_RCC_WWDG1_CLK_ENABLE
    if (map->base == WWDG1) {
        __HAL_RCC_WWDG1_CLK_ENABLE();
    }
#endif
    map->base->CFR = ((0x7FUL << WWDG_CFR_W_Pos) & WWDG_CFR_W) | ((wdgtb << WWDG_CFR_WDGTB_Pos) & WWDG_CFR_WDGTB);
    map->base->CR = WWDG_CR_WDGA | (counter & WWDG_CR_T);
    map->initialized = true;
    return BSP_OK;
}

int32_t bsp_wwdg_refresh(bsp_wwdg_instance_t instance)
{
    if (instance >= BSP_WWDG_MAX) {
        return BSP_ERROR_PARAM;
    }
    wwdg_map_t *map = &s_wwdg_map[instance];
    if (((map->features & BSP_WWDG_FEATURE_REFRESH) == 0U) || (map->base == nullptr)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (!map->initialized) {
        return BSP_ERROR;
    }
    map->base->CR = WWDG_CR_WDGA | 0x7FU;
    return BSP_OK;
}

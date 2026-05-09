#include "bsp_ewdg.h"

typedef struct {
    IWDG_TypeDef *base;
    uint32_t clock_name;
    uint32_t features;
    bool initialized;
} ewdg_map_t;

#define BSP_EWDG_MAP_ITEM(name, base, clk, features) { base, clk, features, false },
static ewdg_map_t s_ewdg_map[BSP_EWDG_MAX] = {
    BSP_EWDG_LIST(BSP_EWDG_MAP_ITEM)
};
#undef BSP_EWDG_MAP_ITEM

static int32_t iwdg_wait_ready(IWDG_TypeDef *base)
{
    for (uint32_t timeout = 0U; timeout < 1000000UL; ++timeout) {
        if ((base->SR & (IWDG_SR_PVU | IWDG_SR_RVU)) == 0U) {
            return BSP_OK;
        }
    }
    return BSP_ERROR_TIMEOUT;
}

static int32_t iwdg_pick_timing(uint32_t timeout_ms, uint32_t *prescaler_code, uint32_t *reload)
{
    static const uint16_t prescalers[] = {4U, 8U, 16U, 32U, 64U, 128U, 256U};
    constexpr uint32_t lsi_hz = 32000U;

    for (uint32_t i = 0U; i < (sizeof(prescalers) / sizeof(prescalers[0])); ++i) {
        const uint64_t ticks = (static_cast<uint64_t>(timeout_ms) * lsi_hz) / (1000ULL * prescalers[i]);
        if ((ticks > 0ULL) && (ticks <= 4096ULL)) {
            *prescaler_code = i;
            *reload = static_cast<uint32_t>(ticks - 1ULL);
            return BSP_OK;
        }
    }
    return BSP_ERROR_PARAM;
}

int32_t bsp_ewdg_init(bsp_ewdg_instance_t instance, uint32_t timeout_ms)
{
    if ((instance >= BSP_EWDG_MAX) || (timeout_ms == 0U)) {
        return BSP_ERROR_PARAM;
    }
    ewdg_map_t *map = &s_ewdg_map[instance];
    if (((map->features & BSP_EWDG_FEATURE_INIT) == 0U) || (map->base == nullptr)) {
        return BSP_ERROR_UNSUPPORTED;
    }

#ifdef __HAL_RCC_LSI_ENABLE
    __HAL_RCC_LSI_ENABLE();
#endif
    uint32_t prescaler_code = 0U;
    uint32_t reload = 0U;
    const int32_t timing_status = iwdg_pick_timing(timeout_ms, &prescaler_code, &reload);
    if (timing_status != BSP_OK) {
        return timing_status;
    }

    IWDG_TypeDef *base = map->base;
    base->KR = 0x5555U;
    if (iwdg_wait_ready(base) != BSP_OK) {
        return BSP_ERROR_TIMEOUT;
    }
    base->PR = prescaler_code & IWDG_PR_PR;
    base->RLR = reload & IWDG_RLR_RL;
    if (iwdg_wait_ready(base) != BSP_OK) {
        return BSP_ERROR_TIMEOUT;
    }
    base->KR = 0xAAAAU;
    base->KR = 0xCCCCU;
    map->initialized = true;
    return BSP_OK;
}

int32_t bsp_ewdg_refresh(bsp_ewdg_instance_t instance)
{
    if (instance >= BSP_EWDG_MAX) {
        return BSP_ERROR_PARAM;
    }
    ewdg_map_t *map = &s_ewdg_map[instance];
    if (((map->features & BSP_EWDG_FEATURE_REFRESH) == 0U) || (map->base == nullptr)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (!map->initialized) {
        return BSP_ERROR;
    }
    map->base->KR = 0xAAAAU;
    return BSP_OK;
}

int32_t bsp_ewdg_deinit(bsp_ewdg_instance_t instance)
{
    if (instance >= BSP_EWDG_MAX) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR_UNSUPPORTED;
}

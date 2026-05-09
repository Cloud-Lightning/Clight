#include "bsp_rtc.h"

typedef struct {
    RTC_TypeDef *base;
    uint32_t features;
    bool initialized;
} rtc_map_t;

#define BSP_RTC_MAP_ITEM(name, base, features) { base, features, false },
static rtc_map_t s_rtc_map[BSP_RTC_MAX] = {
    BSP_RTC_LIST(BSP_RTC_MAP_ITEM)
};
#undef BSP_RTC_MAP_ITEM

static uint8_t bcd_to_bin(uint32_t value)
{
    return static_cast<uint8_t>(((value >> 4U) * 10U) + (value & 0x0FU));
}

static int64_t days_from_civil(int32_t year, uint32_t month, uint32_t day)
{
    year -= (month <= 2U) ? 1 : 0;
    const int32_t era = (year >= 0 ? year : year - 399) / 400;
    const uint32_t yoe = static_cast<uint32_t>(year - era * 400);
    const uint32_t doy = (153U * (month + (month > 2U ? static_cast<uint32_t>(-3) : 9U)) + 2U) / 5U + day - 1U;
    const uint32_t doe = yoe * 365U + yoe / 4U - yoe / 100U + doy;
    return static_cast<int64_t>(era) * 146097LL + static_cast<int64_t>(doe) - 719468LL;
}

static void rtc_unlock(RTC_TypeDef *base)
{
    base->WPR = 0xCAU;
    base->WPR = 0x53U;
}

static void rtc_lock(RTC_TypeDef *base)
{
    base->WPR = 0xFFU;
}

static int32_t rtc_wait_flag_set(volatile uint32_t *reg, uint32_t mask)
{
    for (uint32_t timeout = 0U; timeout < 1000000UL; ++timeout) {
        if ((*reg & mask) != 0U) {
            return BSP_OK;
        }
    }
    return BSP_ERROR_TIMEOUT;
}

static int32_t rtc_enable_lsi_clock(void)
{
#ifdef __HAL_RCC_LSI_ENABLE
    __HAL_RCC_LSI_ENABLE();
#else
    RCC->CSR |= RCC_CSR_LSION;
#endif
    return rtc_wait_flag_set(&RCC->CSR, RCC_CSR_LSIRDY);
}

static int32_t rtc_enter_init_mode(RTC_TypeDef *base)
{
    base->ISR |= RTC_ISR_INIT;
    return rtc_wait_flag_set(&base->ISR, RTC_ISR_INITF);
}

int32_t bsp_rtc_init(bsp_rtc_instance_t instance)
{
    if (instance >= BSP_RTC_MAX) {
        return BSP_ERROR_PARAM;
    }
    rtc_map_t *map = &s_rtc_map[instance];
    if (((map->features & BSP_RTC_FEATURE_UNIX_TIME) == 0U) || (map->base == nullptr)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    PWR->CR1 |= PWR_CR1_DBP;
    const int32_t clock_status = rtc_enable_lsi_clock();
    if (clock_status != BSP_OK) {
        return clock_status;
    }

    if ((RCC->BDCR & RCC_BDCR_RTCSEL) == 0U) {
        RCC->BDCR = (RCC->BDCR & ~RCC_BDCR_RTCSEL) | RCC_BDCR_RTCSEL_1;
    }
#ifdef __HAL_RCC_RTC_ENABLE
    __HAL_RCC_RTC_ENABLE();
#else
    RCC->BDCR |= RCC_BDCR_RTCEN;
#endif

    RTC_TypeDef *base = map->base;
    rtc_unlock(base);
    if ((base->ISR & RTC_ISR_INITS) == 0U) {
        const int32_t init_status = rtc_enter_init_mode(base);
        if (init_status != BSP_OK) {
            rtc_lock(base);
            return init_status;
        }
        base->PRER = (127UL << RTC_PRER_PREDIV_A_Pos) | 249UL;
        base->TR = 0U;
        base->DR = (6UL << RTC_DR_WDU_Pos) | (1UL << RTC_DR_MU_Pos) | (1UL << RTC_DR_DU_Pos);
        base->CR &= ~RTC_CR_FMT;
        base->ISR &= ~RTC_ISR_INIT;
    }
    rtc_lock(base);
    map->initialized = true;
    return BSP_OK;
}

int32_t bsp_rtc_get_unix_time(bsp_rtc_instance_t instance, uint32_t *unix_seconds)
{
    if ((instance >= BSP_RTC_MAX) || (unix_seconds == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    rtc_map_t *map = &s_rtc_map[instance];
    if (((map->features & BSP_RTC_FEATURE_UNIX_TIME) == 0U) || (map->base == nullptr)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (!map->initialized) {
        const int32_t status = bsp_rtc_init(instance);
        if (status != BSP_OK) {
            return status;
        }
    }

    const uint32_t tr = map->base->TR;
    const uint32_t dr = map->base->DR;
    const uint32_t second = bcd_to_bin((tr >> RTC_TR_SU_Pos) & 0x7FU);
    const uint32_t minute = bcd_to_bin((tr >> RTC_TR_MNU_Pos) & 0x7FU);
    const uint32_t hour = bcd_to_bin((tr >> RTC_TR_HU_Pos) & 0x3FU);
    const uint32_t day = bcd_to_bin((dr >> RTC_DR_DU_Pos) & 0x3FU);
    const uint32_t month = bcd_to_bin((dr >> RTC_DR_MU_Pos) & 0x1FU);
    const uint32_t year = 2000U + bcd_to_bin((dr >> RTC_DR_YU_Pos) & 0xFFU);

    if ((month == 0U) || (month > 12U) || (day == 0U) || (day > 31U) || (hour > 23U) || (minute > 59U) || (second > 59U)) {
        return BSP_ERROR;
    }

    const int64_t days = days_from_civil(static_cast<int32_t>(year), month, day);
    if (days < 0) {
        return BSP_ERROR;
    }
    *unix_seconds = static_cast<uint32_t>(days * 86400LL + static_cast<int64_t>(hour * 3600U + minute * 60U + second));
    return BSP_OK;
}

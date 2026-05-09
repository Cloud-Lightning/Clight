#include "bsp_rtc.h"

typedef struct {
    uintptr_t base;
    uint32_t features;
} rtc_map_t;

#define BSP_RTC_MAP_ITEM(name, base, features) { (uintptr_t)(base), features },
static const rtc_map_t s_rtc_map[BSP_RTC_MAX] = {
    BSP_RTC_LIST(BSP_RTC_MAP_ITEM)
};
#undef BSP_RTC_MAP_ITEM

int32_t bsp_rtc_init(bsp_rtc_instance_t instance)
{
    if (instance >= BSP_RTC_MAX) {
        return BSP_ERROR_PARAM;
    }
    return ((s_rtc_map[instance].base != 0U) && ((s_rtc_map[instance].features & BSP_RTC_FEATURE_UNIX_TIME) != 0U)) ? BSP_OK : BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_rtc_get_unix_time(bsp_rtc_instance_t instance, uint32_t *unix_seconds)
{
    if ((instance >= BSP_RTC_MAX) || (unix_seconds == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR_UNSUPPORTED;
}

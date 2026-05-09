#ifndef BSP_RTC_H
#define BSP_RTC_H

#include "bsp_common.h"

#define BSP_RTC_FEATURE_UNIX_TIME (1U << 0)

#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_RTC_LIST
#define BSP_RTC_LIST(X) \
    X(BSP_RTC_MAIN, BSP_RTC_MAIN_BASE, BSP_RTC_FEATURE_UNIX_TIME)
#endif

typedef enum {
#define BSP_RTC_ENUM_ITEM(name, base, features) name,
    BSP_RTC_LIST(BSP_RTC_ENUM_ITEM)
#undef BSP_RTC_ENUM_ITEM
    BSP_RTC_MAX
} bsp_rtc_instance_t;

int32_t bsp_rtc_init(bsp_rtc_instance_t instance);
int32_t bsp_rtc_get_unix_time(bsp_rtc_instance_t instance, uint32_t *unix_seconds);

#ifdef __cplusplus
}
#endif

#endif

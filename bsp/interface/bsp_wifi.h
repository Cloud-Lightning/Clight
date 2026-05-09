#ifndef BSP_WIFI_H
#define BSP_WIFI_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_WIFI_LIST
#define BSP_WIFI_LIST(X) \
    X(BSP_WIFI_MAIN)
#endif

typedef enum {
#define BSP_WIFI_ENUM_ITEM(name) name,
    BSP_WIFI_LIST(BSP_WIFI_ENUM_ITEM)
#undef BSP_WIFI_ENUM_ITEM
    BSP_WIFI_MAX
} bsp_wifi_instance_t;

int32_t bsp_wifi_init(bsp_wifi_instance_t instance);

#ifdef __cplusplus
}
#endif

#endif

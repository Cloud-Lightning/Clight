#ifndef BSP_SDMMC_H
#define BSP_SDMMC_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_SDMMC_LIST
#define BSP_SDMMC_LIST(X) \
    X(BSP_SDMMC0, BSP_SDMMC0_BASE, BSP_SDMMC0_HAS_PINMUX)
#endif

typedef enum {
#define BSP_SDMMC_ENUM_ITEM(name, base, available) name,
    BSP_SDMMC_LIST(BSP_SDMMC_ENUM_ITEM)
#undef BSP_SDMMC_ENUM_ITEM
    BSP_SDMMC_MAX
} bsp_sdmmc_instance_t;

int32_t bsp_sdmmc_init(bsp_sdmmc_instance_t instance);

#ifdef __cplusplus
}
#endif

#endif

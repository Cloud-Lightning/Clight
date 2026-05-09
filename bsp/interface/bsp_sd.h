#ifndef BSP_SD_H
#define BSP_SD_H

#include "bsp_common.h"

#define BSP_SD_FEATURE_INIT        (1U << 0)
#define BSP_SD_FEATURE_BLOCK_READ  (1U << 1)
#define BSP_SD_FEATURE_BLOCK_WRITE (1U << 2)

#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_SD_LIST
#define BSP_SD_LIST(X) \
    X(BSP_SDMMC1, BSP_SDMMC1_BASE, BSP_SDMMC1_HAS_PINMUX, 0U)
#endif

typedef enum {
#define BSP_SD_ENUM_ITEM(name, base, available, features) name,
    BSP_SD_LIST(BSP_SD_ENUM_ITEM)
#undef BSP_SD_ENUM_ITEM
    BSP_SD_MAX
} bsp_sd_instance_t;

int32_t bsp_sd_init(bsp_sd_instance_t instance);
int32_t bsp_sd_read_blocks(bsp_sd_instance_t instance, uint32_t block, uint8_t *data, uint32_t block_count);
int32_t bsp_sd_write_blocks(bsp_sd_instance_t instance, uint32_t block, const uint8_t *data, uint32_t block_count);

#ifdef __cplusplus
}
#endif

#endif

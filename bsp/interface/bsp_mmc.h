#ifndef BSP_MMC_H
#define BSP_MMC_H

#include "bsp_common.h"

#define BSP_MMC_FEATURE_INIT        (1U << 0)
#define BSP_MMC_FEATURE_BLOCK_READ  (1U << 1)
#define BSP_MMC_FEATURE_BLOCK_WRITE (1U << 2)

#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_MMC_LIST
#define BSP_MMC_LIST(X) \
    X(BSP_MMC1, BSP_MMC1_BASE, BSP_MMC1_HAS_PINMUX, 0U)
#endif

typedef enum {
#define BSP_MMC_ENUM_ITEM(name, base, available, features) name,
    BSP_MMC_LIST(BSP_MMC_ENUM_ITEM)
#undef BSP_MMC_ENUM_ITEM
    BSP_MMC_MAX
} bsp_mmc_instance_t;

int32_t bsp_mmc_init(bsp_mmc_instance_t instance);
int32_t bsp_mmc_read_blocks(bsp_mmc_instance_t instance, uint32_t block, uint8_t *data, uint32_t block_count);
int32_t bsp_mmc_write_blocks(bsp_mmc_instance_t instance, uint32_t block, const uint8_t *data, uint32_t block_count);

#ifdef __cplusplus
}
#endif

#endif

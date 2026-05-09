#ifndef BSP_SDIO_H
#define BSP_SDIO_H

#include "bsp_common.h"

#define BSP_SDIO_FEATURE_INIT        (1U << 0)
#define BSP_SDIO_FEATURE_BLOCK_READ  (1U << 1)
#define BSP_SDIO_FEATURE_BLOCK_WRITE (1U << 2)

#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_SDIO_LIST
#define BSP_SDIO_LIST(X) \
    X(BSP_SDIO1, BSP_SDIO1_BASE, BSP_SDIO1_HAS_PINMUX, 0U)
#endif

typedef enum {
#define BSP_SDIO_ENUM_ITEM(name, base, available, features) name,
    BSP_SDIO_LIST(BSP_SDIO_ENUM_ITEM)
#undef BSP_SDIO_ENUM_ITEM
    BSP_SDIO_MAX
} bsp_sdio_instance_t;

int32_t bsp_sdio_init(bsp_sdio_instance_t instance);
int32_t bsp_sdio_read_blocks(bsp_sdio_instance_t instance, uint32_t block, uint8_t *data, uint32_t block_count);
int32_t bsp_sdio_write_blocks(bsp_sdio_instance_t instance, uint32_t block, const uint8_t *data, uint32_t block_count);

#ifdef __cplusplus
}
#endif

#endif

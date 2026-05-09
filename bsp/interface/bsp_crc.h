/**
 * @file bsp_crc.h
 * @brief BSP CRC interface
 */

#ifndef BSP_CRC_H
#define BSP_CRC_H

#include "bsp_common.h"

#define BSP_CRC_FEATURE_CRC32 (1U << 0)

#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_CRC_LIST
#define BSP_CRC_LIST(X) \
    X(BSP_CRC_MAIN, BSP_CRC_MAIN_BASE, BSP_CRC_MAIN_CLK_NAME, BSP_CRC_FEATURE_CRC32)
#endif

typedef enum {
#define BSP_CRC_ENUM_ITEM(name, base, clk, features) name,
    BSP_CRC_LIST(BSP_CRC_ENUM_ITEM)
#undef BSP_CRC_ENUM_ITEM
    BSP_CRC_MAX
} bsp_crc_instance_t;

int32_t bsp_crc32_calculate(bsp_crc_instance_t instance, const uint8_t *data, uint32_t len, uint32_t *crc_out);

#ifdef __cplusplus
}
#endif

#endif /* BSP_CRC_H */

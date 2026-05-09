/**
 * @file bsp_mbx.h
 * @brief BSP MBX interface
 */

#ifndef BSP_MBX_H
#define BSP_MBX_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_MBX_LIST
#define BSP_MBX_LIST(X) \
    X(BSP_MBX_LOCAL, BSP_MBX_LOCAL_BASE, BSP_MBX_LOCAL_CLK_NAME) \
    X(BSP_MBX_REMOTE, BSP_MBX_REMOTE_BASE, BSP_MBX_REMOTE_CLK_NAME)
#endif

typedef enum {
#define BSP_MBX_ENUM_ITEM(name, base, clk) name,
    BSP_MBX_LIST(BSP_MBX_ENUM_ITEM)
#undef BSP_MBX_ENUM_ITEM
    BSP_MBX_MAX
} bsp_mbx_instance_t;

int32_t bsp_mbx_init(bsp_mbx_instance_t instance);
int32_t bsp_mbx_try_send(bsp_mbx_instance_t instance, uint32_t data);
int32_t bsp_mbx_try_receive(bsp_mbx_instance_t instance, uint32_t *data_out);

#ifdef __cplusplus
}
#endif

#endif /* BSP_MBX_H */

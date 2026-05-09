#ifndef BSP_SDSPI_H
#define BSP_SDSPI_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_SDSPI_LIST
#define BSP_SDSPI_LIST(X) \
    X(BSP_SDSPI_MAIN, 0U, 0U)
#endif

typedef enum {
#define BSP_SDSPI_ENUM_ITEM(name, host, has_pinmux) name,
    BSP_SDSPI_LIST(BSP_SDSPI_ENUM_ITEM)
#undef BSP_SDSPI_ENUM_ITEM
    BSP_SDSPI_MAX
} bsp_sdspi_instance_t;

int32_t bsp_sdspi_init(bsp_sdspi_instance_t instance);

#ifdef __cplusplus
}
#endif

#endif

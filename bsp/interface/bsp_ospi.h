#ifndef BSP_OSPI_H
#define BSP_OSPI_H

#include "bsp_common.h"

#define BSP_OSPI_FEATURE_INIT      (1U << 0)
#define BSP_OSPI_FEATURE_MEM_READ  (1U << 1)
#define BSP_OSPI_FEATURE_MEM_WRITE (1U << 2)

#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_OSPI_LIST
#define BSP_OSPI_LIST(X) \
    X(BSP_OSPI1, BSP_OSPI1_BASE, BSP_OSPI1_HAS_PINMUX, 0U)
#endif

typedef enum {
#define BSP_OSPI_ENUM_ITEM(name, base, available, features) name,
    BSP_OSPI_LIST(BSP_OSPI_ENUM_ITEM)
#undef BSP_OSPI_ENUM_ITEM
    BSP_OSPI_MAX
} bsp_ospi_instance_t;

int32_t bsp_ospi_init(bsp_ospi_instance_t instance);
int32_t bsp_ospi_read(bsp_ospi_instance_t instance, uint32_t address, uint8_t *data, uint32_t len);
int32_t bsp_ospi_write(bsp_ospi_instance_t instance, uint32_t address, const uint8_t *data, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif

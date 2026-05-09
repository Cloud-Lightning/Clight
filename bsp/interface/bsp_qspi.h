#ifndef BSP_QSPI_H
#define BSP_QSPI_H

#include "bsp_common.h"

#define BSP_QSPI_FEATURE_INIT      (1U << 0)
#define BSP_QSPI_FEATURE_MEM_READ  (1U << 1)
#define BSP_QSPI_FEATURE_MEM_WRITE (1U << 2)

#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_QSPI_LIST
#define BSP_QSPI_LIST(X) \
    X(BSP_QSPI, BSP_QSPI_BASE, BSP_QSPI_HAS_PINMUX, 0U)
#endif

typedef enum {
#define BSP_QSPI_ENUM_ITEM(name, base, available, features) name,
    BSP_QSPI_LIST(BSP_QSPI_ENUM_ITEM)
#undef BSP_QSPI_ENUM_ITEM
    BSP_QSPI_MAX
} bsp_qspi_instance_t;

int32_t bsp_qspi_init(bsp_qspi_instance_t instance);
int32_t bsp_qspi_read(bsp_qspi_instance_t instance, uint32_t address, uint8_t *data, uint32_t len);
int32_t bsp_qspi_write(bsp_qspi_instance_t instance, uint32_t address, const uint8_t *data, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif

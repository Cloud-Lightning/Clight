#ifndef BSP_DAC_H
#define BSP_DAC_H

#include "bsp_common.h"

#define BSP_DAC_FEATURE_DIRECT_WRITE (1U << 0)

#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_DAC_LIST
#define BSP_DAC_LIST(X) \
    X(BSP_DAC_MAIN, BSP_DAC_MAIN_BASE, BSP_DAC_MAIN_HAS_PINMUX, BSP_DAC_FEATURE_DIRECT_WRITE)
#endif

typedef enum {
#define BSP_DAC_ENUM_ITEM(name, base, has_pinmux, features) name,
    BSP_DAC_LIST(BSP_DAC_ENUM_ITEM)
#undef BSP_DAC_ENUM_ITEM
    BSP_DAC_MAX
} bsp_dac_instance_t;

int32_t bsp_dac_init(bsp_dac_instance_t instance);
int32_t bsp_dac_write(bsp_dac_instance_t instance, uint8_t channel, uint32_t value);

#ifdef __cplusplus
}
#endif

#endif

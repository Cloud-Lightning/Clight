#ifndef BSP_SMARTCARD_H
#define BSP_SMARTCARD_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_SMARTCARD_LIST
#define BSP_SMARTCARD_LIST(X) \
    X(BSP_SMARTCARD_USART1, BSP_SMARTCARD_USART1_BASE, BSP_SMARTCARD_USART1_HAS_PINMUX)
#endif

typedef enum {
#define BSP_SMARTCARD_ENUM_ITEM(name, base, available) name,
    BSP_SMARTCARD_LIST(BSP_SMARTCARD_ENUM_ITEM)
#undef BSP_SMARTCARD_ENUM_ITEM
    BSP_SMARTCARD_MAX
} bsp_smartcard_instance_t;

int32_t bsp_smartcard_init(bsp_smartcard_instance_t instance);

#ifdef __cplusplus
}
#endif

#endif

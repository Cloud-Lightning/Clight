#ifndef BSP_USART_H
#define BSP_USART_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_USART_LIST
#define BSP_USART_LIST(X) \
    X(BSP_USART1, BSP_USART1_BASE, BSP_USART1_HAS_PINMUX)
#endif

typedef enum {
#define BSP_USART_ENUM_ITEM(name, base, available) name,
    BSP_USART_LIST(BSP_USART_ENUM_ITEM)
#undef BSP_USART_ENUM_ITEM
    BSP_USART_MAX
} bsp_usart_instance_t;

int32_t bsp_usart_init(bsp_usart_instance_t instance);

#ifdef __cplusplus
}
#endif

#endif

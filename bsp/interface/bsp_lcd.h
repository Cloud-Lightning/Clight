#ifndef BSP_LCD_H
#define BSP_LCD_H

#include "bsp_common.h"
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_LCD_LIST
#define BSP_LCD_LIST(X) \
    X(BSP_LCD_MAIN, 0U, 0U)
#endif

typedef enum {
#define BSP_LCD_ENUM_ITEM(name, bus, has_pinmux) name,
    BSP_LCD_LIST(BSP_LCD_ENUM_ITEM)
#undef BSP_LCD_ENUM_ITEM
    BSP_LCD_MAX
} bsp_lcd_instance_t;

int32_t bsp_lcd_init(bsp_lcd_instance_t instance);

#ifdef __cplusplus
}
#endif

#endif

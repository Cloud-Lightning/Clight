#include "bsp_lcd.h"

int32_t bsp_lcd_init(bsp_lcd_instance_t instance)
{
    if (instance >= BSP_LCD_MAX) {
        return BSP_ERROR_PARAM;
    }

    return BSP_ERROR_UNSUPPORTED;
}

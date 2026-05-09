#pragma once

#include "DisplayDevice.hpp"

#if __has_include("lvgl.h")
#include "lvgl.h"
#define LVGL_DISPLAY_AVAILABLE 1
#elif __has_include("lvgl/lvgl.h")
#include "lvgl/lvgl.h"
#define LVGL_DISPLAY_AVAILABLE 1
#else
#define LVGL_DISPLAY_AVAILABLE 0
#endif

class LvglDisplay {
public:
    explicit LvglDisplay(DisplayDevice &display);

    Status init();
    Status flush(std::uint16_t x, std::uint16_t y, std::uint16_t width, std::uint16_t height, ByteView bitmap);

#if LVGL_DISPLAY_AVAILABLE
    void bind(lv_display_t *display);
    static void flushCallback(lv_display_t *display, const lv_area_t *area, std::uint8_t *bitmap);
#endif

private:
    DisplayDevice &display_;
};

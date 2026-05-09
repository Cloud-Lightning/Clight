#include "LvglDisplay.hpp"

LvglDisplay::LvglDisplay(DisplayDevice &display)
    : display_(display)
{
}

Status LvglDisplay::init()
{
    return display_.init();
}

Status LvglDisplay::flush(std::uint16_t x,
                          std::uint16_t y,
                          std::uint16_t width,
                          std::uint16_t height,
                          ByteView bitmap)
{
    return display_.drawBitmap(x, y, width, height, bitmap);
}

#if LVGL_DISPLAY_AVAILABLE
void LvglDisplay::bind(lv_display_t *display)
{
    lv_display_set_user_data(display, this);
    lv_display_set_flush_cb(display, &LvglDisplay::flushCallback);
}

void LvglDisplay::flushCallback(lv_display_t *display, const lv_area_t *area, std::uint8_t *bitmap)
{
    auto *self = static_cast<LvglDisplay *>(lv_display_get_user_data(display));
    if (self == nullptr) {
        lv_display_flush_ready(display);
        return;
    }

    const auto width = static_cast<std::uint16_t>(area->x2 - area->x1 + 1);
    const auto height = static_cast<std::uint16_t>(area->y2 - area->y1 + 1);
    self->flush(static_cast<std::uint16_t>(area->x1),
                static_cast<std::uint16_t>(area->y1),
                width,
                height,
                ByteView(bitmap, static_cast<std::size_t>(width) * height * 2U));
    lv_display_flush_ready(display);
}
#endif

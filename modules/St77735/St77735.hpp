#pragma once

#include "DisplayDevice.hpp"
#include "St7789.hpp"

class St77735 : public DisplayDevice {
public:
    St77735(Spi &bus,
            Gpio &chipSelect,
            Gpio &dataCommand,
            Gpio *reset = nullptr,
            Gpio *backlight = nullptr,
            std::uint16_t width = 240U,
            std::uint16_t height = 320U,
            std::uint32_t busFrequencyHz = 0U,
            St7789::DelayCallback delay = nullptr);

    Status init() override { return display_.init(); }
    Status clear() override { return display_.clear(); }
    Status fill(std::uint16_t color) override { return display_.fill(color); }
    Status drawPixel(std::uint16_t x, std::uint16_t y, std::uint16_t color) override
    {
        return display_.drawPixel(x, y, color);
    }
    Status drawBitmap(std::uint16_t x,
                      std::uint16_t y,
                      std::uint16_t width,
                      std::uint16_t height,
                      ByteView bitmap) override
    {
        return display_.drawBitmap(x, y, width, height, bitmap);
    }
    Status update() override { return display_.update(); }
    DisplaySize size() const override { return display_.size(); }
    PixelFormat pixelFormat() const override { return display_.pixelFormat(); }

private:
    St7789 display_;
};

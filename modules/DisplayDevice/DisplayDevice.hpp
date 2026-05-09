#pragma once

#include "Common.hpp"
#include "DeviceTypes.hpp"

class DisplayDevice {
public:
    virtual ~DisplayDevice() = default;

    virtual Status init() = 0;
    virtual Status clear() = 0;
    virtual Status fill(std::uint16_t color) = 0;
    virtual Status drawPixel(std::uint16_t x, std::uint16_t y, std::uint16_t color) = 0;
    virtual Status drawBitmap(std::uint16_t x,
                              std::uint16_t y,
                              std::uint16_t width,
                              std::uint16_t height,
                              ByteView bitmap) = 0;
    virtual Status update() = 0;
    virtual DisplaySize size() const = 0;
    virtual PixelFormat pixelFormat() const = 0;
};

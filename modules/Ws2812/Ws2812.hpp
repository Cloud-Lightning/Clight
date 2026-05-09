#pragma once

#include <cstddef>
#include <span>

#include "Common.hpp"
#include "DeviceTypes.hpp"
#include "Spi.hpp"
#include "Ws2812Ops.hpp"

class Ws2812 {
public:
    Ws2812(Spi &bus, std::span<Rgb888> pixels);
    Ws2812(const Ws2812Ops &ops, void *context, std::span<Rgb888> pixels);

    Status init();
    void setPixel(std::size_t index, Rgb888 color);
    void fill(Rgb888 color);
    void clear();
    Status refresh();
    bool busy() const;
    std::size_t size() const { return pixels_.size(); }

private:
    const Ws2812Ops *ops_;
    void *context_;
    std::span<Rgb888> pixels_;
};

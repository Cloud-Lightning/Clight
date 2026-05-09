#pragma once

#include <span>

#include "Common.hpp"
#include "DeviceTypes.hpp"

struct Ws2812Ops {
    Status (*init)(void *context);
    Status (*write)(void *context, std::span<const Rgb888> pixels);
    bool (*busy)(void *context);
};

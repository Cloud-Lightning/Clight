#pragma once

#include <cstdint>

#include "Common.hpp"

class TinyUsbDeviceCore {
public:
    static Status init(std::uint8_t rhport = 0U);
    static void poll();
    static bool initialized();
    static bool mounted();
};

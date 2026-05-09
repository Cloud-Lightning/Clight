#pragma once

#include <array>
#include <cstdint>

#include "Common.hpp"

class TinyUsbHidDevice {
public:
    explicit TinyUsbHidDevice(std::uint8_t rhport = 0U)
        : rhport_(rhport)
    {
    }

    Status init();
    void poll() const;
    bool mounted() const;
    Status sendKeyboard(std::uint8_t modifier, const std::array<std::uint8_t, 6> &keycodes) const;
    Status sendMouse(std::uint8_t buttons, std::int8_t x, std::int8_t y, std::int8_t wheel = 0, std::int8_t pan = 0) const;

private:
    std::uint8_t rhport_;
    bool initialized_ = false;
};

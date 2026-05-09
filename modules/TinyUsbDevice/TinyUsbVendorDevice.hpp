#pragma once

#include <cstdint>

#include "Common.hpp"

class TinyUsbVendorDevice {
public:
    explicit TinyUsbVendorDevice(std::uint8_t rhport = 0U)
        : rhport_(rhport)
    {
    }

    Status init();
    void poll() const;
    bool mounted() const;
    std::uint32_t available() const;
    std::uint32_t read(ByteSpan data) const;
    Status write(ByteView data) const;
    void flush() const;

private:
    std::uint8_t rhport_;
    bool initialized_ = false;
};

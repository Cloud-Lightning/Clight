#pragma once

#include <cstdint>

#include "Common.hpp"

class TinyUsbCdc {
public:
    explicit TinyUsbCdc(std::uint8_t rhport = 0U)
        : rhport_(rhport)
    {
    }

    Status init();
    void poll() const;

    bool initialized() const { return initialized_; }
    bool mounted() const;
    bool connected(std::uint8_t interfaceIndex = 0U) const;
    std::uint32_t available(std::uint8_t interfaceIndex = 0U) const;
    std::uint32_t read(ByteSpan data, std::uint8_t interfaceIndex = 0U) const;
    Status write(ByteView data, std::uint8_t interfaceIndex = 0U) const;
    void flush(std::uint8_t interfaceIndex = 0U) const;

private:
    std::uint8_t rhport_;
    bool initialized_ = false;
};

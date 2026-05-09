#pragma once

#include <cstdint>

#include "Common.hpp"

class TinyUsbHost {
public:
    using MscMountedCallback = void (*)(std::uint8_t devAddr, void *arg);
    using HidInputCallback = void (*)(std::uint8_t devAddr, std::uint8_t instance, ByteView report, void *arg);

    explicit TinyUsbHost(std::uint8_t rhport = 0U)
        : rhport_(rhport)
    {
    }

    Status init();
    void poll() const;
    bool initialized() const { return initialized_; }
    Status onMscMounted(MscMountedCallback callback, void *arg = nullptr);
    Status onHidInput(HidInputCallback callback, void *arg = nullptr);

private:
    std::uint8_t rhport_;
    bool initialized_ = false;
};

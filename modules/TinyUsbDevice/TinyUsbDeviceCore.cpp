#include "TinyUsbDeviceCore.hpp"

extern "C" {
#include "TinyUsbPlatform.h"
#include "tusb.h"
}

namespace {
bool s_initialized = false;
std::uint8_t s_rhport = 0U;
} // namespace

Status TinyUsbDeviceCore::init(std::uint8_t rhport)
{
    if (s_initialized) {
        return (rhport == s_rhport) ? Status::Ok : Status::Unsupported;
    }

    if (!tinyusb_platform_init(rhport)) {
        return Status::Unsupported;
    }

    tusb_rhport_init_t device_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_AUTO,
    };

    if (!tusb_init(rhport, &device_init)) {
        return Status::Error;
    }

    tinyusb_platform_after_init();
    s_initialized = true;
    s_rhport = rhport;
    return Status::Ok;
}

void TinyUsbDeviceCore::poll()
{
    if (s_initialized) {
        tud_task();
    }
}

bool TinyUsbDeviceCore::initialized()
{
    return s_initialized;
}

bool TinyUsbDeviceCore::mounted()
{
    return s_initialized && tud_mounted();
}

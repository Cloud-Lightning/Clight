#include "TinyUsbVendorDevice.hpp"

#include "TinyUsbDeviceCore.hpp"

extern "C" {
#include "tusb.h"
}

Status TinyUsbVendorDevice::init()
{
#if CFG_TUD_VENDOR
    const auto status = TinyUsbDeviceCore::init(rhport_);
    initialized_ = (status == Status::Ok);
    return status;
#else
    return Status::Unsupported;
#endif
}

void TinyUsbVendorDevice::poll() const
{
    if (initialized_) {
        TinyUsbDeviceCore::poll();
    }
}

bool TinyUsbVendorDevice::mounted() const
{
    return initialized_ && TinyUsbDeviceCore::mounted();
}

std::uint32_t TinyUsbVendorDevice::available() const
{
#if CFG_TUD_VENDOR
    return initialized_ ? tud_vendor_available() : 0U;
#else
    return 0U;
#endif
}

std::uint32_t TinyUsbVendorDevice::read(ByteSpan data) const
{
#if CFG_TUD_VENDOR
    if (!initialized_ || data.empty()) {
        return 0U;
    }
    return tud_vendor_read(data.data(), static_cast<std::uint32_t>(data.size()));
#else
    (void)data;
    return 0U;
#endif
}

Status TinyUsbVendorDevice::write(ByteView data) const
{
#if CFG_TUD_VENDOR
    if (!initialized_) {
        return Status::Error;
    }
    if (data.empty()) {
        return Status::Ok;
    }
    const auto written = tud_vendor_write(data.data(), static_cast<std::uint32_t>(data.size()));
    tud_vendor_flush();
    return (written == data.size()) ? Status::Ok : Status::Busy;
#else
    (void)data;
    return Status::Unsupported;
#endif
}

void TinyUsbVendorDevice::flush() const
{
#if CFG_TUD_VENDOR
    if (initialized_) {
        tud_vendor_flush();
    }
#endif
}

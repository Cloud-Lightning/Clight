#include "TinyUsbCdc.hpp"

#include "TinyUsbDeviceCore.hpp"

extern "C" {
#include "tusb.h"
}

Status TinyUsbCdc::init()
{
    if (initialized_) {
        return Status::Ok;
    }

    const auto status = TinyUsbDeviceCore::init(rhport_);
    initialized_ = (status == Status::Ok);
    return status;
}

void TinyUsbCdc::poll() const
{
    if (initialized_) {
        TinyUsbDeviceCore::poll();
    }
}

bool TinyUsbCdc::mounted() const
{
    return initialized_ && TinyUsbDeviceCore::mounted();
}

bool TinyUsbCdc::connected(std::uint8_t interfaceIndex) const
{
    return initialized_ && tud_cdc_n_connected(interfaceIndex);
}

std::uint32_t TinyUsbCdc::available(std::uint8_t interfaceIndex) const
{
    if (!initialized_) {
        return 0U;
    }
    return tud_cdc_n_available(interfaceIndex);
}

std::uint32_t TinyUsbCdc::read(ByteSpan data, std::uint8_t interfaceIndex) const
{
    if (!initialized_ || data.empty()) {
        return 0U;
    }
    return tud_cdc_n_read(interfaceIndex, data.data(), static_cast<std::uint32_t>(data.size()));
}

Status TinyUsbCdc::write(ByteView data, std::uint8_t interfaceIndex) const
{
    if (!initialized_) {
        return Status::Error;
    }
    if (data.empty()) {
        return Status::Ok;
    }

    const std::uint32_t written = tud_cdc_n_write(interfaceIndex, data.data(), static_cast<std::uint32_t>(data.size()));
    tud_cdc_n_write_flush(interfaceIndex);
    return (written == data.size()) ? Status::Ok : Status::Busy;
}

void TinyUsbCdc::flush(std::uint8_t interfaceIndex) const
{
    if (initialized_) {
        tud_cdc_n_write_flush(interfaceIndex);
    }
}

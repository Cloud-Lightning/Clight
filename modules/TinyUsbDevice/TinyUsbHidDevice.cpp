#include "TinyUsbHidDevice.hpp"

#include "TinyUsbDeviceCore.hpp"

extern "C" {
#include "tusb.h"
}

namespace {
constexpr std::uint8_t kKeyboardReportId = 1U;
constexpr std::uint8_t kMouseReportId = 2U;
}

Status TinyUsbHidDevice::init()
{
#if CFG_TUD_HID
    const auto status = TinyUsbDeviceCore::init(rhport_);
    initialized_ = (status == Status::Ok);
    return status;
#else
    return Status::Unsupported;
#endif
}

void TinyUsbHidDevice::poll() const
{
    if (initialized_) {
        TinyUsbDeviceCore::poll();
    }
}

bool TinyUsbHidDevice::mounted() const
{
    return initialized_ && TinyUsbDeviceCore::mounted();
}

Status TinyUsbHidDevice::sendKeyboard(std::uint8_t modifier, const std::array<std::uint8_t, 6> &keycodes) const
{
#if CFG_TUD_HID
    if (!initialized_) {
        return Status::Error;
    }
    if (!tud_hid_ready()) {
        return Status::Busy;
    }
    return tud_hid_keyboard_report(kKeyboardReportId, modifier, keycodes.data()) ? Status::Ok : Status::Busy;
#else
    (void)modifier;
    (void)keycodes;
    return Status::Unsupported;
#endif
}

Status TinyUsbHidDevice::sendMouse(std::uint8_t buttons, std::int8_t x, std::int8_t y, std::int8_t wheel, std::int8_t pan) const
{
#if CFG_TUD_HID
    if (!initialized_) {
        return Status::Error;
    }
    if (!tud_hid_ready()) {
        return Status::Busy;
    }
    return tud_hid_mouse_report(kMouseReportId, buttons, x, y, wheel, pan) ? Status::Ok : Status::Busy;
#else
    (void)buttons;
    (void)x;
    (void)y;
    (void)wheel;
    (void)pan;
    return Status::Unsupported;
#endif
}

#if CFG_TUD_HID
extern "C" uint16_t tud_hid_get_report_cb(uint8_t, uint8_t, hid_report_type_t, uint8_t *, uint16_t)
{
    return 0;
}

extern "C" void tud_hid_set_report_cb(uint8_t, uint8_t, hid_report_type_t, uint8_t const *, uint16_t)
{
}
#endif

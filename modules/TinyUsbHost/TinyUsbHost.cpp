#include "TinyUsbHost.hpp"

extern "C" {
#if defined(CLIGHT_TINYUSB_HOST_ENABLE)
#include "TinyUsbPlatform.h"
#include "tusb.h"
#endif
}

namespace {
TinyUsbHost::MscMountedCallback s_msc_mounted_callback = nullptr;
void *s_msc_mounted_arg = nullptr;
TinyUsbHost::HidInputCallback s_hid_input_callback = nullptr;
void *s_hid_input_arg = nullptr;
} // namespace

Status TinyUsbHost::init()
{
#if defined(CLIGHT_TINYUSB_HOST_ENABLE) && CFG_TUH_ENABLED
    if (!tinyusb_platform_init_host(rhport_)) {
        return Status::Unsupported;
    }
    tusb_rhport_init_t host_init = {
        .role = TUSB_ROLE_HOST,
        .speed = TUSB_SPEED_AUTO,
    };
    if (!tusb_init(rhport_, &host_init)) {
        return Status::Error;
    }
    initialized_ = true;
    return Status::Ok;
#else
    return Status::Unsupported;
#endif
}

void TinyUsbHost::poll() const
{
#if defined(CLIGHT_TINYUSB_HOST_ENABLE) && CFG_TUH_ENABLED
    if (initialized_) {
        tuh_task();
    }
#endif
}

Status TinyUsbHost::onMscMounted(MscMountedCallback callback, void *arg)
{
    s_msc_mounted_callback = callback;
    s_msc_mounted_arg = arg;
    return Status::Ok;
}

Status TinyUsbHost::onHidInput(HidInputCallback callback, void *arg)
{
    s_hid_input_callback = callback;
    s_hid_input_arg = arg;
    return Status::Ok;
}

#if defined(CLIGHT_TINYUSB_HOST_ENABLE) && CFG_TUH_ENABLED
extern "C" void tuh_msc_mount_cb(uint8_t dev_addr)
{
    if (s_msc_mounted_callback != nullptr) {
        s_msc_mounted_callback(dev_addr, s_msc_mounted_arg);
    }
}

extern "C" void tuh_msc_umount_cb(uint8_t)
{
}

extern "C" void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *, uint16_t)
{
    (void)tuh_hid_receive_report(dev_addr, instance);
}

extern "C" void tuh_hid_umount_cb(uint8_t, uint8_t)
{
}

extern "C" void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len)
{
    if ((s_hid_input_callback != nullptr) && (report != nullptr)) {
        s_hid_input_callback(dev_addr, instance, ByteView(report, len), s_hid_input_arg);
    }
    (void)tuh_hid_receive_report(dev_addr, instance);
}
#endif

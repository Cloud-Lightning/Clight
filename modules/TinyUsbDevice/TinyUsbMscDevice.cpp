#include "TinyUsbMscDevice.hpp"

#include <algorithm>
#include <cstring>

#include "TinyUsbDeviceCore.hpp"

extern "C" {
#include "tusb.h"
}

namespace {
TinyUsbBlockDevice *s_block_device = nullptr;

Status range_check(TinyUsbBlockDevice &device, std::uint32_t lba, std::uint32_t offset, std::uint32_t size)
{
    if (device.blockSize() == 0U) {
        return Status::Param;
    }
    const std::uint64_t start = static_cast<std::uint64_t>(lba) * device.blockSize() + offset;
    const std::uint64_t end = start + size;
    const std::uint64_t total = static_cast<std::uint64_t>(device.blockCount()) * device.blockSize();
    return (end <= total) ? Status::Ok : Status::Param;
}
} // namespace

std::uint32_t TinyUsbRamDisk::blockCount() const
{
    return (blockSize_ == 0U) ? 0U : static_cast<std::uint32_t>(storage_.size() / blockSize_);
}

Status TinyUsbRamDisk::read(std::uint32_t lba, std::uint32_t offset, ByteSpan data)
{
    if (range_check(*this, lba, offset, static_cast<std::uint32_t>(data.size())) != Status::Ok) {
        return Status::Param;
    }
    const auto byte_offset = static_cast<std::size_t>(lba) * blockSize_ + offset;
    std::copy_n(storage_.data() + byte_offset, data.size(), data.data());
    return Status::Ok;
}

Status TinyUsbRamDisk::write(std::uint32_t lba, std::uint32_t offset, ByteView data)
{
    if (range_check(*this, lba, offset, static_cast<std::uint32_t>(data.size())) != Status::Ok) {
        return Status::Param;
    }
    const auto byte_offset = static_cast<std::size_t>(lba) * blockSize_ + offset;
    std::copy_n(data.data(), data.size(), storage_.data() + byte_offset);
    return Status::Ok;
}

Status TinyUsbMscDevice::init(TinyUsbBlockDevice &blockDevice)
{
#if CFG_TUD_MSC
    if (blockDevice.blockCount() == 0U || blockDevice.blockSize() == 0U) {
        return Status::Param;
    }
    s_block_device = &blockDevice;
    const auto status = TinyUsbDeviceCore::init(rhport_);
    initialized_ = (status == Status::Ok);
    return status;
#else
    (void)blockDevice;
    return Status::Unsupported;
#endif
}

void TinyUsbMscDevice::poll() const
{
    if (initialized_) {
        TinyUsbDeviceCore::poll();
    }
}

bool TinyUsbMscDevice::mounted() const
{
    return initialized_ && TinyUsbDeviceCore::mounted();
}

#if CFG_TUD_MSC
extern "C" uint8_t tud_msc_get_maxlun_cb(void)
{
    return 0U;
}

extern "C" void tud_msc_inquiry_cb(uint8_t, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4])
{
    std::memcpy(vendor_id, "Clight  ", 8);
    std::memcpy(product_id, "RAM Disk        ", 16);
    std::memcpy(product_rev, "1.0 ", 4);
}

extern "C" bool tud_msc_test_unit_ready_cb(uint8_t)
{
    return (s_block_device != nullptr) && s_block_device->ready();
}

extern "C" void tud_msc_capacity_cb(uint8_t, uint32_t *block_count, uint16_t *block_size)
{
    if ((s_block_device == nullptr) || (block_count == nullptr) || (block_size == nullptr)) {
        return;
    }
    *block_count = s_block_device->blockCount();
    *block_size = s_block_device->blockSize();
}

extern "C" bool tud_msc_is_writable_cb(uint8_t)
{
    return (s_block_device != nullptr) && s_block_device->writable();
}

extern "C" int32_t tud_msc_read10_cb(uint8_t, uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize)
{
    if ((s_block_device == nullptr) || (buffer == nullptr)) {
        return -1;
    }
    const auto status = s_block_device->read(lba, offset, ByteSpan(static_cast<std::uint8_t *>(buffer), bufsize));
    return (status == Status::Ok) ? static_cast<int32_t>(bufsize) : -1;
}

extern "C" int32_t tud_msc_write10_cb(uint8_t, uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize)
{
    if ((s_block_device == nullptr) || (buffer == nullptr)) {
        return -1;
    }
    const auto status = s_block_device->write(lba, offset, ByteView(buffer, bufsize));
    return (status == Status::Ok) ? static_cast<int32_t>(bufsize) : -1;
}

extern "C" void tud_msc_write10_complete_cb(uint8_t)
{
}

extern "C" int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void *buffer, uint16_t bufsize)
{
    (void)buffer;
    (void)bufsize;
    if (scsi_cmd[0] == SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL) {
        return 0;
    }
    tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
    return -1;
}
#endif

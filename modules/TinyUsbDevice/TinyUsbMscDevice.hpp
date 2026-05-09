#pragma once

#include <cstdint>

#include "Common.hpp"

class TinyUsbBlockDevice {
public:
    virtual ~TinyUsbBlockDevice() = default;
    virtual std::uint32_t blockCount() const = 0;
    virtual std::uint16_t blockSize() const = 0;
    virtual bool ready() const { return true; }
    virtual bool writable() const { return true; }
    virtual Status read(std::uint32_t lba, std::uint32_t offset, ByteSpan data) = 0;
    virtual Status write(std::uint32_t lba, std::uint32_t offset, ByteView data) = 0;
};

class TinyUsbRamDisk final : public TinyUsbBlockDevice {
public:
    TinyUsbRamDisk(ByteSpan storage, std::uint16_t blockSize = 512U)
        : storage_(storage)
        , blockSize_(blockSize)
    {
    }

    std::uint32_t blockCount() const override;
    std::uint16_t blockSize() const override { return blockSize_; }
    Status read(std::uint32_t lba, std::uint32_t offset, ByteSpan data) override;
    Status write(std::uint32_t lba, std::uint32_t offset, ByteView data) override;

private:
    ByteSpan storage_;
    std::uint16_t blockSize_;
};

class TinyUsbMscDevice {
public:
    explicit TinyUsbMscDevice(std::uint8_t rhport = 0U)
        : rhport_(rhport)
    {
    }

    Status init(TinyUsbBlockDevice &blockDevice);
    void poll() const;
    bool mounted() const;

private:
    std::uint8_t rhport_;
    bool initialized_ = false;
};

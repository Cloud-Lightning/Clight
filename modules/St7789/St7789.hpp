#pragma once

#include "DisplayDevice.hpp"
#include "Gpio.hpp"
#include "Spi.hpp"

class St7789 : public DisplayDevice {
public:
    using DelayCallback = void (*)(std::uint32_t ms);

    St7789(Spi &bus,
           Gpio &chipSelect,
           Gpio &dataCommand,
           Gpio *reset = nullptr,
           Gpio *backlight = nullptr,
           std::uint16_t width = 240U,
           std::uint16_t height = 240U,
           std::uint8_t memoryAccessControl = 0x00U,
           std::uint16_t xOffset = 0U,
           std::uint16_t yOffset = 0U,
           std::uint32_t busFrequencyHz = 0U,
           DelayCallback delay = nullptr);

    Status init() override;
    Status clear() override;
    Status fill(std::uint16_t color) override;
    Status drawPixel(std::uint16_t x, std::uint16_t y, std::uint16_t color) override;
    Status drawBitmap(std::uint16_t x,
                      std::uint16_t y,
                      std::uint16_t width,
                      std::uint16_t height,
                      ByteView bitmap) override;
    Status update() override { return Status::Ok; }
    DisplaySize size() const override { return size_; }
    PixelFormat pixelFormat() const override { return PixelFormat::Rgb565; }

private:
    Status writeCommand(std::uint8_t command) const;
    Status writeData(ByteView data) const;
    Status writeDataByte(std::uint8_t data) const;
    Status setWindow(std::uint16_t x, std::uint16_t y, std::uint16_t width, std::uint16_t height) const;
    void delayMs(std::uint32_t ms) const;

    Spi &bus_;
    Gpio &chipSelect_;
    Gpio &dataCommand_;
    Gpio *reset_;
    Gpio *backlight_;
    DisplaySize size_{};
    std::uint8_t memoryAccessControl_ = 0x00U;
    std::uint16_t xOffset_ = 0U;
    std::uint16_t yOffset_ = 0U;
    std::uint32_t busFrequencyHz_ = 0U;
    DelayCallback delay_ = nullptr;
};

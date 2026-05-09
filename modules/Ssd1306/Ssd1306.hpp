#pragma once

#include <array>
#include <string_view>

#include "DisplayDevice.hpp"
#include "Gpio.hpp"
#include "I2c.hpp"

class Ssd1306 : public DisplayDevice {
public:
    using DelayCallback = void (*)(std::uint32_t ms);

    explicit Ssd1306(I2c &bus,
                     std::uint16_t address = 0x3CU,
                     Gpio *reset = nullptr,
                     std::uint16_t width = 128U,
                     std::uint16_t height = 64U,
                     std::uint32_t busFrequencyHz = 0U);

    Status init() override;
    Status init(bool configureBus);
    Status clear() override;
    Status fill(std::uint16_t color) override;
    Status clearBuffer(std::uint16_t color = 0U);
    Status drawPixel(std::uint16_t x, std::uint16_t y, std::uint16_t color) override;
    Status drawChar(std::uint16_t x, std::uint16_t y, char ch, std::uint16_t color = 1U);
    Status drawString(std::uint16_t x, std::uint16_t y, std::string_view text, std::uint16_t color = 1U);
    Status drawBitmap(std::uint16_t x,
                      std::uint16_t y,
                      std::uint16_t width,
                      std::uint16_t height,
                      ByteView bitmap) override;
    Status update() override;
    Status pageTurnTo(ByteView nextFrame,
                      std::uint32_t frameDelayMs = 20U,
                      DelayCallback delay = nullptr,
                      bool leftToRight = true);
    DisplaySize size() const override { return size_; }
    PixelFormat pixelFormat() const override { return PixelFormat::Mono1; }

private:
    std::size_t pageCount() const;
    std::size_t bufferSize() const;
    Status sendCommand(std::uint8_t command) const;
    Status sendCommands(ByteView commands) const;
    Status loadFrame(ByteView frame);
    void copyColumnFromFrame(std::uint16_t destColumn, ByteView frame, std::uint16_t srcColumn);
    void fillColumn(std::uint16_t column, std::uint8_t value);

    I2c &bus_;
    std::uint16_t address_;
    Gpio *reset_;
    DisplaySize size_{};
    std::uint32_t busFrequencyHz_ = 0U;
    std::array<std::uint8_t, 1024> buffer_{};
};

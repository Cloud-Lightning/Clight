#include "St7789.hpp"

#include <array>

namespace {

constexpr std::uint8_t kCmdSleepOut = 0x11U;
constexpr std::uint8_t kCmdDisplayOn = 0x29U;
constexpr std::uint8_t kCmdMemoryAccessControl = 0x36U;
constexpr std::uint8_t kCmdPixelFormat = 0x3AU;
constexpr std::uint8_t kCmdColumnAddressSet = 0x2AU;
constexpr std::uint8_t kCmdRowAddressSet = 0x2BU;
constexpr std::uint8_t kCmdMemoryWrite = 0x2CU;
constexpr std::uint8_t kCmdPorchControl = 0xB2U;
constexpr std::uint8_t kCmdGateControl = 0xB7U;
constexpr std::uint8_t kCmdVcomSetting = 0xBBU;
constexpr std::uint8_t kCmdLcmControl = 0xC0U;
constexpr std::uint8_t kCmdVdvVrhEnable = 0xC2U;
constexpr std::uint8_t kCmdVrhSet = 0xC3U;
constexpr std::uint8_t kCmdVdvSet = 0xC4U;
constexpr std::uint8_t kCmdFrameRateControl = 0xC6U;
constexpr std::uint8_t kCmdPowerControl = 0xD0U;
constexpr std::uint8_t kCmdPositiveGamma = 0xE0U;
constexpr std::uint8_t kCmdNegativeGamma = 0xE1U;
constexpr std::uint8_t kCmdDisplayInversionOff = 0x20U;

} // namespace

St7789::St7789(Spi &bus,
               Gpio &chipSelect,
               Gpio &dataCommand,
               Gpio *reset,
               Gpio *backlight,
               std::uint16_t width,
               std::uint16_t height,
               std::uint8_t memoryAccessControl,
               std::uint16_t xOffset,
               std::uint16_t yOffset,
               std::uint32_t busFrequencyHz,
               DelayCallback delay)
    : bus_(bus),
      chipSelect_(chipSelect),
      dataCommand_(dataCommand),
      reset_(reset),
      backlight_(backlight),
      size_{width, height},
      memoryAccessControl_(memoryAccessControl),
      xOffset_(xOffset),
      yOffset_(yOffset),
      busFrequencyHz_(busFrequencyHz),
      delay_(delay)
{
}

Status St7789::init()
{
    auto status = bus_.init(SpiMode::Mode0, busFrequencyHz_);
    if (status != Status::Ok) {
        return status;
    }

    status = chipSelect_.initOutput();
    if (status != Status::Ok) {
        return status;
    }
    status = dataCommand_.initOutput();
    if (status != Status::Ok) {
        return status;
    }
    chipSelect_.write(true);
    dataCommand_.write(true);

    if (reset_ != nullptr) {
        status = reset_->initOutput();
        if (status != Status::Ok) {
            return status;
        }
        reset_->write(false);
        delayMs(20U);
        reset_->write(true);
        delayMs(120U);
    }
    if (backlight_ != nullptr) {
        status = backlight_->initOutput();
        if (status != Status::Ok) {
            return status;
        }
        backlight_->write(true);
    }

    status = writeCommand(kCmdSleepOut);
    if (status != Status::Ok) {
        return status;
    }
    delayMs(120U);
    {
        const std::array<std::uint8_t, 5> porch = {0x0CU, 0x0CU, 0x00U, 0x33U, 0x33U};
        status = writeCommand(kCmdPorchControl);
        if (status != Status::Ok) {
            return status;
        }
        status = writeData(ByteView(porch.data(), porch.size()));
        if (status != Status::Ok) {
            return status;
        }
    }
    status = writeCommand(kCmdGateControl);
    if (status != Status::Ok || writeDataByte(0x35U) != Status::Ok) {
        return Status::Error;
    }
    status = writeCommand(kCmdVcomSetting);
    if (status != Status::Ok || writeDataByte(0x19U) != Status::Ok) {
        return Status::Error;
    }
    status = writeCommand(kCmdLcmControl);
    if (status != Status::Ok || writeDataByte(0x2CU) != Status::Ok) {
        return Status::Error;
    }
    status = writeCommand(kCmdVdvVrhEnable);
    if (status != Status::Ok || writeDataByte(0x01U) != Status::Ok) {
        return Status::Error;
    }
    status = writeCommand(kCmdVrhSet);
    if (status != Status::Ok || writeDataByte(0x12U) != Status::Ok) {
        return Status::Error;
    }
    status = writeCommand(kCmdVdvSet);
    if (status != Status::Ok || writeDataByte(0x20U) != Status::Ok) {
        return Status::Error;
    }
    status = writeCommand(kCmdFrameRateControl);
    if (status != Status::Ok || writeDataByte(0x0FU) != Status::Ok) {
        return Status::Error;
    }
    {
        const std::array<std::uint8_t, 2> power = {0xA4U, 0xA1U};
        status = writeCommand(kCmdPowerControl);
        if (status != Status::Ok) {
            return status;
        }
        status = writeData(ByteView(power.data(), power.size()));
        if (status != Status::Ok) {
            return status;
        }
    }
    {
        const std::array<std::uint8_t, 1> madctl = {memoryAccessControl_};
        status = writeCommand(kCmdMemoryAccessControl);
        if (status != Status::Ok) {
            return status;
        }
        status = writeData(ByteView(madctl.data(), madctl.size()));
        if (status != Status::Ok) {
            return status;
        }
    }
    {
        const std::array<std::uint8_t, 1> pixelFormat = {0x55U};
        status = writeCommand(kCmdPixelFormat);
        if (status != Status::Ok) {
            return status;
        }
        status = writeData(ByteView(pixelFormat.data(), pixelFormat.size()));
        if (status != Status::Ok) {
            return status;
        }
    }
    {
        const std::array<std::uint8_t, 14> gamma = {
            0xD0U, 0x04U, 0x0DU, 0x11U, 0x13U, 0x2BU, 0x3FU,
            0x54U, 0x4CU, 0x18U, 0x0DU, 0x0BU, 0x1FU, 0x23U
        };
        status = writeCommand(kCmdPositiveGamma);
        if (status != Status::Ok) {
            return status;
        }
        status = writeData(ByteView(gamma.data(), gamma.size()));
        if (status != Status::Ok) {
            return status;
        }
    }
    {
        const std::array<std::uint8_t, 14> gamma = {
            0xD0U, 0x04U, 0x0CU, 0x11U, 0x13U, 0x2CU, 0x3FU,
            0x44U, 0x51U, 0x2FU, 0x1FU, 0x1FU, 0x20U, 0x23U
        };
        status = writeCommand(kCmdNegativeGamma);
        if (status != Status::Ok) {
            return status;
        }
        status = writeData(ByteView(gamma.data(), gamma.size()));
        if (status != Status::Ok) {
            return status;
        }
    }
    status = writeCommand(kCmdDisplayInversionOff);
    if (status != Status::Ok) {
        return status;
    }
    status = writeCommand(kCmdDisplayOn);
    if (status != Status::Ok) {
        return status;
    }
    delayMs(20U);
    return clear();
}

Status St7789::clear()
{
    return fill(0x0000U);
}

Status St7789::fill(std::uint16_t color)
{
    auto status = setWindow(0U, 0U, size_.width, size_.height);
    if (status != Status::Ok) {
        return status;
    }
    status = writeCommand(kCmdMemoryWrite);
    if (status != Status::Ok) {
        return status;
    }

    std::array<std::uint8_t, 128> chunk{};
    for (std::size_t index = 0; index < chunk.size(); index += 2U) {
        chunk[index] = static_cast<std::uint8_t>(color >> 8U);
        chunk[index + 1U] = static_cast<std::uint8_t>(color & 0xFFU);
    }

    const std::uint32_t totalPixels = static_cast<std::uint32_t>(size_.width) * size_.height;
    std::uint32_t remaining = totalPixels;
    while (remaining > 0U) {
        const std::uint32_t chunkPixels = (remaining > (chunk.size() / 2U)) ? (chunk.size() / 2U) : remaining;
        status = writeData(ByteView(chunk.data(), chunkPixels * 2U));
        if (status != Status::Ok) {
            return status;
        }
        remaining -= chunkPixels;
    }
    return Status::Ok;
}

Status St7789::drawPixel(std::uint16_t x, std::uint16_t y, std::uint16_t color)
{
    if (x >= size_.width || y >= size_.height) {
        return Status::Param;
    }

    const auto status = setWindow(x, y, 1U, 1U);
    if (status != Status::Ok) {
        return status;
    }
    const std::array<std::uint8_t, 2> pixel = {
        static_cast<std::uint8_t>(color >> 8U),
        static_cast<std::uint8_t>(color & 0xFFU),
    };
    if (writeCommand(kCmdMemoryWrite) != Status::Ok) {
        return Status::Error;
    }
    return writeData(ByteView(pixel.data(), pixel.size()));
}

Status St7789::drawBitmap(std::uint16_t x,
                          std::uint16_t y,
                          std::uint16_t width,
                          std::uint16_t height,
                          ByteView bitmap)
{
    if ((x + width) > size_.width || (y + height) > size_.height) {
        return Status::Param;
    }
    if (bitmap.size() < static_cast<std::size_t>(width) * height * 2U) {
        return Status::Param;
    }

    auto status = setWindow(x, y, width, height);
    if (status != Status::Ok) {
        return status;
    }
    status = writeCommand(kCmdMemoryWrite);
    if (status != Status::Ok) {
        return status;
    }
    return writeData(bitmap);
}

Status St7789::writeCommand(std::uint8_t command) const
{
    chipSelect_.write(false);
    dataCommand_.write(false);
    const auto status = bus_.write(ByteView(&command, 1U));
    chipSelect_.write(true);
    return status;
}

Status St7789::writeData(ByteView data) const
{
    chipSelect_.write(false);
    dataCommand_.write(true);
    const auto status = bus_.write(data);
    chipSelect_.write(true);
    return status;
}

Status St7789::writeDataByte(std::uint8_t data) const
{
    return writeData(ByteView(&data, 1U));
}

Status St7789::setWindow(std::uint16_t x, std::uint16_t y, std::uint16_t width, std::uint16_t height) const
{
    const std::uint16_t xStart = static_cast<std::uint16_t>(x + xOffset_);
    const std::uint16_t xEnd = static_cast<std::uint16_t>(x + width - 1U + xOffset_);
    const std::uint16_t yStart = static_cast<std::uint16_t>(y + yOffset_);
    const std::uint16_t yEnd = static_cast<std::uint16_t>(y + height - 1U + yOffset_);

    std::array<std::uint8_t, 4> payload = {
        static_cast<std::uint8_t>(xStart >> 8U),
        static_cast<std::uint8_t>(xStart & 0xFFU),
        static_cast<std::uint8_t>(xEnd >> 8U),
        static_cast<std::uint8_t>(xEnd & 0xFFU),
    };
    auto status = writeCommand(kCmdColumnAddressSet);
    if (status != Status::Ok) {
        return status;
    }
    status = writeData(ByteView(payload.data(), payload.size()));
    if (status != Status::Ok) {
        return status;
    }

    payload = {
        static_cast<std::uint8_t>(yStart >> 8U),
        static_cast<std::uint8_t>(yStart & 0xFFU),
        static_cast<std::uint8_t>(yEnd >> 8U),
        static_cast<std::uint8_t>(yEnd & 0xFFU),
    };
    status = writeCommand(kCmdRowAddressSet);
    if (status != Status::Ok) {
        return status;
    }
    return writeData(ByteView(payload.data(), payload.size()));
}

void St7789::delayMs(std::uint32_t ms) const
{
    if (delay_ != nullptr) {
        delay_(ms);
        return;
    }

    for (std::uint32_t loops = ms * 8000U; loops > 0U; --loops) {
        __asm__ volatile("" ::: "memory");
    }
}

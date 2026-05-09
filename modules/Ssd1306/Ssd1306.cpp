#include "Ssd1306.hpp"

#include <array>

#include "Ssd1306Font.hpp"

namespace {

constexpr std::uint8_t kControlCommand = 0x00U;
constexpr std::uint8_t kControlData = 0x40U;
constexpr std::uint8_t kDisplayOff = 0xAEU;
constexpr std::uint8_t kDisplayOn = 0xAFU;
constexpr std::uint8_t kSetContrast = 0x81U;
constexpr std::uint8_t kDisplayAllOnResume = 0xA4U;
constexpr std::uint8_t kNormalDisplay = 0xA6U;
constexpr std::uint8_t kSetDisplayOffset = 0xD3U;
constexpr std::uint8_t kSetComPins = 0xDAU;
constexpr std::uint8_t kSetVcomDetect = 0xDBU;
constexpr std::uint8_t kSetDisplayClockDiv = 0xD5U;
constexpr std::uint8_t kSetPrecharge = 0xD9U;
constexpr std::uint8_t kSetMultiplex = 0xA8U;
constexpr std::uint8_t kSetStartLine = 0x40U;
constexpr std::uint8_t kMemoryMode = 0x20U;
constexpr std::uint8_t kColumnAddr = 0x21U;
constexpr std::uint8_t kPageAddr = 0x22U;
constexpr std::uint8_t kComScanDec = 0xC8U;
constexpr std::uint8_t kSegRemap = 0xA0U;
constexpr std::uint8_t kChargePump = 0x8DU;
constexpr std::uint8_t kDeactivateScroll = 0x2EU;
constexpr std::uint16_t kFontWidth = 6U;
constexpr std::uint16_t kFontHeight = 8U;

} // namespace

Ssd1306::Ssd1306(I2c &bus, std::uint16_t address, Gpio *reset, std::uint16_t width, std::uint16_t height, std::uint32_t busFrequencyHz)
    : bus_(bus), address_(address), reset_(reset), size_{width, height}, busFrequencyHz_(busFrequencyHz)
{
}

Status Ssd1306::init()
{
    return init(true);
}

Status Ssd1306::init(bool configureBus)
{
    if (configureBus) {
        auto status = bus_.init(busFrequencyHz_, false, TransferMode::Blocking);
        if (status != Status::Ok) {
            return status;
        }
    }

    auto status = Status::Ok;

    if (reset_ != nullptr) {
        status = reset_->initOutput();
        if (status != Status::Ok) {
            return status;
        }
        reset_->write(false);
        reset_->write(true);
    }

    const std::array<std::uint8_t, 28> initSequence = {
        kControlCommand,
        kDisplayOff,
        kSetDisplayClockDiv, 0x80U,
        kSetMultiplex, static_cast<std::uint8_t>(size_.height - 1U),
        kSetDisplayOffset, 0x00U,
        static_cast<std::uint8_t>(kSetStartLine | 0x00U),
        kChargePump, 0x14U,
        kMemoryMode, 0x00U,
        static_cast<std::uint8_t>(kSegRemap | 0x01U),
        kComScanDec,
        kSetComPins, static_cast<std::uint8_t>(size_.height > 32U ? 0x12U : 0x02U),
        kSetContrast, 0xCFU,
        kSetPrecharge, 0xF1U,
        kSetVcomDetect, 0x40U,
        kDisplayAllOnResume,
        kNormalDisplay,
        kDeactivateScroll,
        kDisplayOn,
    };

    status = bus_.write(address_, ByteView(initSequence.data(), initSequence.size()));
    if (status != Status::Ok) {
        return status;
    }

    return clear();
}

Status Ssd1306::clear()
{
    clearBuffer(0U);
    return update();
}

Status Ssd1306::fill(std::uint16_t color)
{
    clearBuffer(color);
    return update();
}

Status Ssd1306::clearBuffer(std::uint16_t color)
{
    buffer_.fill((color == 0U) ? 0x00U : 0xFFU);
    return Status::Ok;
}

Status Ssd1306::drawPixel(std::uint16_t x, std::uint16_t y, std::uint16_t color)
{
    if (x >= size_.width || y >= size_.height) {
        return Status::Param;
    }

    const std::size_t index = static_cast<std::size_t>(x) + (static_cast<std::size_t>(y / 8U) * size_.width);
    const std::uint8_t mask = static_cast<std::uint8_t>(1U << (y % 8U));
    if (color == 0U) {
        buffer_[index] &= static_cast<std::uint8_t>(~mask);
    } else {
        buffer_[index] |= mask;
    }
    return Status::Ok;
}

Status Ssd1306::drawChar(std::uint16_t x, std::uint16_t y, char ch, std::uint16_t color)
{
    if (x >= size_.width || y >= size_.height) {
        return Status::Param;
    }

    const char printable = (ch < ' ' || ch > '~') ? ' ' : ch;
    const auto index = static_cast<std::size_t>(printable - ' ');
    for (std::uint16_t column = 0; column < kFontWidth; ++column) {
        const auto bits = kSsd1306Font6x8[index][column];
        for (std::uint16_t row = 0; row < kFontHeight; ++row) {
            if ((bits & static_cast<std::uint8_t>(1U << row)) != 0U) {
                const auto status = drawPixel(x + column, y + row, color);
                if (status != Status::Ok) {
                    return status;
                }
            }
        }
    }
    return Status::Ok;
}

Status Ssd1306::drawString(std::uint16_t x, std::uint16_t y, std::string_view text, std::uint16_t color)
{
    std::uint16_t cursorX = x;
    std::uint16_t cursorY = y;
    const std::uint16_t startX = x;

    for (const char ch : text) {
        if (ch == '\n') {
            cursorX = startX;
            cursorY = static_cast<std::uint16_t>(cursorY + kFontHeight);
            continue;
        }
        if ((cursorX + kFontWidth) > size_.width || (cursorY + kFontHeight) > size_.height) {
            break;
        }
        const auto status = drawChar(cursorX, cursorY, ch, color);
        if (status != Status::Ok) {
            return status;
        }
        cursorX = static_cast<std::uint16_t>(cursorX + kFontWidth);
    }
    return Status::Ok;
}

Status Ssd1306::drawBitmap(std::uint16_t x,
                           std::uint16_t y,
                           std::uint16_t width,
                           std::uint16_t height,
                           ByteView bitmap)
{
    if ((x + width) > size_.width || (y + height) > size_.height) {
        return Status::Param;
    }

    const std::size_t required = static_cast<std::size_t>((width * height) + 7U) / 8U;
    if (bitmap.size() < required) {
        return Status::Param;
    }

    std::size_t bitIndex = 0U;
    for (std::uint16_t row = 0; row < height; ++row) {
        for (std::uint16_t col = 0; col < width; ++col) {
            const auto byte = bitmap[bitIndex / 8U];
            const auto bit = static_cast<std::uint8_t>(0x80U >> (bitIndex % 8U));
            const auto status = drawPixel(x + col, y + row, (byte & bit) != 0U ? 1U : 0U);
            if (status != Status::Ok) {
                return status;
            }
            ++bitIndex;
        }
    }
    return Status::Ok;
}

Status Ssd1306::update()
{
    const std::array<std::uint8_t, 3> columnAddress = {kColumnAddr, 0x00U, static_cast<std::uint8_t>(size_.width - 1U)};
    auto status = sendCommands(ByteView(columnAddress.data(), columnAddress.size()));
    if (status != Status::Ok) {
        return status;
    }
    const std::array<std::uint8_t, 3> pageAddress = {kPageAddr, 0x00U, static_cast<std::uint8_t>(pageCount() - 1U)};
    status = sendCommands(ByteView(pageAddress.data(), pageAddress.size()));
    if (status != Status::Ok) {
        return status;
    }

    std::array<std::uint8_t, 33> packet{};
    packet[0] = kControlData;
    for (std::size_t index = 0; index < bufferSize(); index += 32U) {
        const std::size_t chunk = ((index + 32U) <= bufferSize()) ? 32U : (bufferSize() - index);
        for (std::size_t offset = 0; offset < chunk; ++offset) {
            packet[offset + 1U] = buffer_[index + offset];
        }
        status = bus_.write(address_, ByteView(packet.data(), chunk + 1U));
        if (status != Status::Ok) {
            return status;
        }
    }
    return Status::Ok;
}

Status Ssd1306::pageTurnTo(ByteView nextFrame,
                           std::uint32_t frameDelayMs,
                           DelayCallback delay,
                           bool leftToRight)
{
    if (nextFrame.size() < bufferSize()) {
        return Status::Param;
    }

    std::array<std::uint8_t, 1024> currentFrame = buffer_;
    const ByteView current(currentFrame.data(), bufferSize());
    const std::uint16_t center = static_cast<std::uint16_t>(size_.width / 2U);

    for (std::uint16_t step = 0; step <= size_.width; ++step) {
        const bool openingNext = step >= center;
        const std::uint16_t phase = openingNext ? static_cast<std::uint16_t>(step - center) : step;
        const std::uint16_t visibleWidth = openingNext ? phase : static_cast<std::uint16_t>(center - phase);
        const std::uint16_t leftEdge = static_cast<std::uint16_t>(center - visibleWidth);
        const std::uint16_t rightEdge = static_cast<std::uint16_t>(center + visibleWidth);

        buffer_.fill(0x00U);

        if (!openingNext) {
            for (std::uint16_t offset = 0; offset < visibleWidth; ++offset) {
                const std::uint16_t leftDest = static_cast<std::uint16_t>(leftEdge + offset);
                const std::uint16_t rightDest = static_cast<std::uint16_t>(center + offset);
                const std::uint16_t leftSrc = leftToRight ? offset : static_cast<std::uint16_t>((center - visibleWidth) + offset);
                const std::uint16_t rightSrc = leftToRight ? static_cast<std::uint16_t>(center + offset)
                                                           : static_cast<std::uint16_t>(size_.width - visibleWidth + offset);
                copyColumnFromFrame(leftDest, current, leftSrc);
                if (rightDest < size_.width) {
                    copyColumnFromFrame(rightDest, current, rightSrc);
                }
            }
        } else {
            for (std::uint16_t offset = 0; offset < visibleWidth; ++offset) {
                const std::uint16_t leftDest = static_cast<std::uint16_t>(leftEdge + offset);
                const std::uint16_t rightDest = static_cast<std::uint16_t>(center + offset);
                const std::uint16_t leftSrc = leftToRight ? static_cast<std::uint16_t>((center - visibleWidth) + offset) : offset;
                const std::uint16_t rightSrc = leftToRight ? static_cast<std::uint16_t>(size_.width - visibleWidth + offset)
                                                           : static_cast<std::uint16_t>(center + offset);
                copyColumnFromFrame(leftDest, nextFrame, leftSrc);
                if (rightDest < size_.width) {
                    copyColumnFromFrame(rightDest, nextFrame, rightSrc);
                }
            }
        }

        if (leftEdge > 0U) {
            fillColumn(static_cast<std::uint16_t>(leftEdge - 1U), 0x18U);
        }
        if (rightEdge < size_.width) {
            fillColumn(rightEdge, 0x18U);
        }

        const auto status = update();
        if (status != Status::Ok) {
            loadFrame(current);
            return status;
        }

        if (delay != nullptr && frameDelayMs > 0U && step < size_.width) {
            delay(frameDelayMs);
        }
    }

    return loadFrame(nextFrame);
}

std::size_t Ssd1306::pageCount() const
{
    return size_.height / 8U;
}

std::size_t Ssd1306::bufferSize() const
{
    return static_cast<std::size_t>(size_.width) * pageCount();
}

Status Ssd1306::loadFrame(ByteView frame)
{
    if (frame.size() < bufferSize()) {
        return Status::Param;
    }
    for (std::size_t index = 0; index < bufferSize(); ++index) {
        buffer_[index] = frame[index];
    }
    return update();
}

void Ssd1306::copyColumnFromFrame(std::uint16_t destColumn, ByteView frame, std::uint16_t srcColumn)
{
    if (destColumn >= size_.width || srcColumn >= size_.width || frame.size() < bufferSize()) {
        return;
    }

    for (std::size_t page = 0; page < pageCount(); ++page) {
        buffer_[destColumn + (page * size_.width)] = frame[srcColumn + (page * size_.width)];
    }
}

void Ssd1306::fillColumn(std::uint16_t column, std::uint8_t value)
{
    if (column >= size_.width) {
        return;
    }

    for (std::size_t page = 0; page < pageCount(); ++page) {
        buffer_[column + (page * size_.width)] = value;
    }
}

Status Ssd1306::sendCommand(std::uint8_t command) const
{
    const std::array<std::uint8_t, 2> payload = {kControlCommand, command};
    return bus_.write(address_, ByteView(payload.data(), payload.size()));
}

Status Ssd1306::sendCommands(ByteView commands) const
{
    std::array<std::uint8_t, 16> payload = {};
    if ((commands.size() + 1U) > payload.size()) {
        return Status::Param;
    }
    payload[0] = kControlCommand;
    for (std::size_t index = 0; index < commands.size(); ++index) {
        payload[index + 1U] = commands[index];
    }
    return bus_.write(address_, ByteView(payload.data(), commands.size() + 1U));
}

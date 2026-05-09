#include "SdCard.hpp"

#include <array>

namespace {

constexpr std::uint8_t kCmd0 = 0U;
constexpr std::uint8_t kCmd8 = 8U;
constexpr std::uint8_t kCmd17 = 17U;
constexpr std::uint8_t kCmd24 = 24U;
constexpr std::uint8_t kCmd55 = 55U;
constexpr std::uint8_t kCmd58 = 58U;
constexpr std::uint8_t kAcmd41 = 41U;

} // namespace

SdCard::SdCard(Spi &bus, Gpio &chipSelect)
    : bus_(bus), chipSelect_(chipSelect)
{
}

Status SdCard::init()
{
    auto status = bus_.init();
    if (status != Status::Ok) {
        return status;
    }
    status = chipSelect_.initOutput();
    if (status != Status::Ok) {
        return status;
    }
    chipSelect_.write(true);

    std::array<std::uint8_t, 10> idleClocks{};
    idleClocks.fill(0xFFU);
    status = bus_.write(ByteView(idleClocks.data(), idleClocks.size()));
    if (status != Status::Ok) {
        return status;
    }

    std::uint8_t response = 0xFFU;
    status = sendCommand(kCmd0, 0U, 0x95U, response);
    if (status != Status::Ok || response != 0x01U) {
        return (status == Status::Ok) ? Status::Error : status;
    }

    status = sendCommand(kCmd8, 0x1AAU, 0x87U, response);
    if (status != Status::Ok) {
        return status;
    }

    for (std::uint32_t attempt = 0; attempt < 100U; ++attempt) {
        status = sendCommand(kCmd55, 0U, 0x01U, response);
        if (status != Status::Ok) {
            return status;
        }
        status = sendCommand(kAcmd41, 0x40000000U, 0x01U, response);
        if (status != Status::Ok) {
            return status;
        }
        if (response == 0x00U) {
            break;
        }
        if (attempt == 99U) {
            return Status::Timeout;
        }
    }

    return sendCommand(kCmd58, 0U, 0x01U, response);
}

Status SdCard::readBlock(std::uint32_t blockIndex, ByteSpan block)
{
    if (block.size() < 512U) {
        return Status::Param;
    }

    std::uint8_t response = 0xFFU;
    auto status = sendCommand(kCmd17, blockIndex, 0x01U, response);
    if (status != Status::Ok) {
        return status;
    }
    if (response != 0x00U) {
        return Status::Error;
    }

    std::uint8_t token = 0xFFU;
    status = waitReady(1000U, token);
    if (status != Status::Ok) {
        return status;
    }
    if (token != 0xFEU) {
        return Status::Timeout;
    }

    for (std::size_t index = 0; index < 512U; ++index) {
        status = transfer(0xFFU, block[index]);
        if (status != Status::Ok) {
            return status;
        }
    }
    std::uint8_t crc = 0U;
    transfer(0xFFU, crc);
    transfer(0xFFU, crc);
    chipSelect_.write(true);
    return Status::Ok;
}

Status SdCard::writeBlock(std::uint32_t blockIndex, ByteView block)
{
    if (block.size() < 512U) {
        return Status::Param;
    }

    std::uint8_t response = 0xFFU;
    auto status = sendCommand(kCmd24, blockIndex, 0x01U, response);
    if (status != Status::Ok) {
        return status;
    }
    if (response != 0x00U) {
        return Status::Error;
    }

    transfer(0xFEU, response);
    for (std::size_t index = 0; index < 512U; ++index) {
        status = transfer(block[index], response);
        if (status != Status::Ok) {
            return status;
        }
    }
    transfer(0xFFU, response);
    transfer(0xFFU, response);
    status = transfer(0xFFU, response);
    chipSelect_.write(true);
    return status;
}

Status SdCard::waitReady(std::uint32_t attempts, std::uint8_t &value) const
{
    for (std::uint32_t attempt = 0; attempt < attempts; ++attempt) {
        const auto status = transfer(0xFFU, value);
        if (status != Status::Ok) {
            return status;
        }
        if (value != 0xFFU) {
            return Status::Ok;
        }
    }
    return Status::Timeout;
}

Status SdCard::sendCommand(std::uint8_t command,
                           std::uint32_t argument,
                           std::uint8_t crc,
                           std::uint8_t &response) const
{
    chipSelect_.write(false);
    std::array<std::uint8_t, 6> frame = {
        static_cast<std::uint8_t>(0x40U | command),
        static_cast<std::uint8_t>(argument >> 24U),
        static_cast<std::uint8_t>(argument >> 16U),
        static_cast<std::uint8_t>(argument >> 8U),
        static_cast<std::uint8_t>(argument),
        crc,
    };
    std::array<std::uint8_t, 6> rx{};
    auto status = bus_.transfer(ByteView(frame.data(), frame.size()), ByteSpan(rx.data(), rx.size()));
    if (status != Status::Ok) {
        chipSelect_.write(true);
        return status;
    }
    status = waitReady(16U, response);
    if (status != Status::Ok) {
        chipSelect_.write(true);
        return status;
    }
    return Status::Ok;
}

Status SdCard::transfer(std::uint8_t tx, std::uint8_t &rx) const
{
    const std::array<std::uint8_t, 1> txBuffer = {tx};
    std::array<std::uint8_t, 1> rxBuffer{};
    const auto status = bus_.transfer(ByteView(txBuffer.data(), txBuffer.size()), ByteSpan(rxBuffer.data(), rxBuffer.size()));
    rx = rxBuffer[0];
    return status;
}

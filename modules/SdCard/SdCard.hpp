#pragma once

#include "Common.hpp"
#include "Gpio.hpp"
#include "Spi.hpp"

class SdCard {
public:
    explicit SdCard(Spi &bus, Gpio &chipSelect);

    Status init();
    Status readBlock(std::uint32_t blockIndex, ByteSpan block);
    Status writeBlock(std::uint32_t blockIndex, ByteView block);

private:
    Status waitReady(std::uint32_t attempts, std::uint8_t &value) const;
    Status sendCommand(std::uint8_t command,
                       std::uint32_t argument,
                       std::uint8_t crc,
                       std::uint8_t &response) const;
    Status transfer(std::uint8_t tx, std::uint8_t &rx) const;

    Spi &bus_;
    Gpio &chipSelect_;
};

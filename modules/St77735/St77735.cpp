#include "St77735.hpp"

St77735::St77735(Spi &bus,
                 Gpio &chipSelect,
                 Gpio &dataCommand,
                 Gpio *reset,
                 Gpio *backlight,
                 std::uint16_t width,
                 std::uint16_t height,
                 std::uint32_t busFrequencyHz,
                 St7789::DelayCallback delay)
    : display_(bus, chipSelect, dataCommand, reset, backlight, width, height, 0x00U, 0U, 0U, busFrequencyHz, delay)
{
}

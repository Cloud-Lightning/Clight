#include "Mpu6050.hpp"

#include <array>

namespace {

constexpr std::uint8_t kWhoAmIReg = 0x75U;
constexpr std::uint8_t kExpectedWhoAmI = 0x68U;
constexpr std::uint8_t kPowerMgmt1Reg = 0x6BU;
constexpr std::uint8_t kAccelConfigReg = 0x1CU;
constexpr std::uint8_t kGyroConfigReg = 0x1BU;
constexpr std::uint8_t kSampleStartReg = 0x3BU;

std::int16_t readBigEndian16(const std::uint8_t *data)
{
    return static_cast<std::int16_t>((static_cast<std::uint16_t>(data[0]) << 8U) |
                                     static_cast<std::uint16_t>(data[1]));
}

} // namespace

Mpu6050::Mpu6050(I2c &bus, std::uint16_t address)
    : bus_(bus), address_(address)
{
}

Status Mpu6050::init()
{
    auto status = bus_.init();
    if (status != Status::Ok) {
        return status;
    }

    std::array<std::uint8_t, 1> whoAmI{};
    status = readBlock(kWhoAmIReg, whoAmI);
    if (status != Status::Ok) {
        return status;
    }
    if (whoAmI[0] != kExpectedWhoAmI) {
        return Status::Error;
    }

    status = writeRegister(kPowerMgmt1Reg, 0x01U);
    if (status != Status::Ok) {
        return status;
    }
    status = setAccelRange(0x00U);
    if (status != Status::Ok) {
        return status;
    }
    return setGyroRange(0x00U);
}

Status Mpu6050::update()
{
    std::array<std::uint8_t, 14> raw{};
    const auto status = readBlock(kSampleStartReg, raw);
    if (status != Status::Ok) {
        return status;
    }

    constexpr float kAccelScale = 2.0f * 9.80665f / 32768.0f;
    constexpr float kGyroScale = 250.0f / 32768.0f;

    sample_.accel.x = static_cast<float>(readBigEndian16(&raw[0])) * kAccelScale;
    sample_.accel.y = static_cast<float>(readBigEndian16(&raw[2])) * kAccelScale;
    sample_.accel.z = static_cast<float>(readBigEndian16(&raw[4])) * kAccelScale;
    sample_.temperature = static_cast<float>(readBigEndian16(&raw[6])) / 340.0f + 36.53f;
    sample_.gyro.x = static_cast<float>(readBigEndian16(&raw[8])) * kGyroScale;
    sample_.gyro.y = static_cast<float>(readBigEndian16(&raw[10])) * kGyroScale;
    sample_.gyro.z = static_cast<float>(readBigEndian16(&raw[12])) * kGyroScale;
    return Status::Ok;
}

Status Mpu6050::setAccelRange(std::uint8_t value)
{
    return writeRegister(kAccelConfigReg, value);
}

Status Mpu6050::setGyroRange(std::uint8_t value)
{
    return writeRegister(kGyroConfigReg, value);
}

Status Mpu6050::readBlock(std::uint8_t reg, ByteSpan data) const
{
    return bus_.readRegister(address_, reg, data);
}

Status Mpu6050::writeRegister(std::uint8_t reg, std::uint8_t value) const
{
    return bus_.writeRegister(address_, reg, value);
}

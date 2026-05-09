#pragma once

#include "I2c.hpp"
#include "ImuDevice.hpp"

class Mpu6050 : public ImuDevice {
public:
    explicit Mpu6050(I2c &bus, std::uint16_t address = 0x68U);

    Status init() override;
    Status update() override;
    Vec3f accel() const override { return sample_.accel; }
    Vec3f gyro() const override { return sample_.gyro; }
    float temperature() const override { return sample_.temperature; }

    Status setAccelRange(std::uint8_t value);
    Status setGyroRange(std::uint8_t value);

private:
    Status readBlock(std::uint8_t reg, ByteSpan data) const;
    Status writeRegister(std::uint8_t reg, std::uint8_t value) const;

    I2c &bus_;
    std::uint16_t address_;
    ImuSample sample_{};
};

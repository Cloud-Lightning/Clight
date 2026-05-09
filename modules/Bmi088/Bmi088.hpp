#pragma once

#include "DisplayDevice.hpp"
#include "Gpio.hpp"
#include "ImuDevice.hpp"
#include "Spi.hpp"

class Bmi088 : public ImuDevice {
public:
    Bmi088(Spi &spi, Gpio &accelCs, Gpio &gyroCs);

    Status init() override;
    Status update() override;
    Vec3f accel() const override { return sample_.accel; }
    Vec3f gyro() const override { return sample_.gyro; }
    float temperature() const override { return sample_.temperature; }

    Status setAccelRange(std::uint8_t value);
    Status setGyroRange(std::uint8_t value);

private:
    Status readAccelBlock(std::uint8_t reg, ByteSpan data) const;
    Status readGyroBlock(std::uint8_t reg, ByteSpan data) const;
    Status writeAccelRegister(std::uint8_t reg, std::uint8_t value) const;
    Status writeGyroRegister(std::uint8_t reg, std::uint8_t value) const;
    Status transfer(Gpio &chipSelect, ByteView tx, ByteSpan rx) const;

    Spi &spi_;
    Gpio &accelCs_;
    Gpio &gyroCs_;
    float accelScale_ = 0.0f;
    float gyroScale_ = 0.0f;
    ImuSample sample_{};
};

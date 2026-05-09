#pragma once

#include "Common.hpp"
#include "DeviceTypes.hpp"

class ImuDevice {
public:
    virtual ~ImuDevice() = default;

    virtual Status init() = 0;
    virtual Status update() = 0;
    virtual Vec3f accel() const = 0;
    virtual Vec3f gyro() const = 0;
    virtual float temperature() const = 0;

    ImuSample sample() const
    {
        return ImuSample{accel(), gyro(), temperature()};
    }
};

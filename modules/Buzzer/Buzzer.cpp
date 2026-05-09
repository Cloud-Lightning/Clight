#include "Buzzer.hpp"

Buzzer::Buzzer(Pwm &pwm, std::uint8_t channel) : pwm_(pwm), channel_(channel) {}

Status Buzzer::init()
{
    const auto initStatus = pwm_.init(1000U, PwmAlign::Edge);
    if (initStatus != Status::Ok) {
        return initStatus;
    }
    const auto dutyStatus = pwm_.duty(channel_, 0.0f);
    if (dutyStatus != Status::Ok) {
        return dutyStatus;
    }
    return pwm_.output(channel_, false);
}

Status Buzzer::tone(std::uint32_t frequencyHz, float dutyPercent)
{
    const auto initStatus = pwm_.init(frequencyHz, PwmAlign::Edge);
    if (initStatus != Status::Ok) {
        return initStatus;
    }
    const auto dutyStatus = pwm_.duty(channel_, dutyPercent);
    if (dutyStatus != Status::Ok) {
        return dutyStatus;
    }
    const auto outputStatus = pwm_.output(channel_, true);
    if (outputStatus != Status::Ok) {
        return outputStatus;
    }
    return pwm_.start();
}

Status Buzzer::off()
{
    const auto dutyStatus = pwm_.duty(channel_, 0.0f);
    if (dutyStatus != Status::Ok) {
        return dutyStatus;
    }
    const auto outputStatus = pwm_.output(channel_, false);
    if (outputStatus != Status::Ok) {
        return outputStatus;
    }
    return pwm_.stop();
}

#pragma once

#include <cstdint>

#include "Pwm.hpp"

class Buzzer {
public:
    Buzzer(Pwm &pwm, std::uint8_t channel);

    Status init();
    Status tone(std::uint32_t frequencyHz, float dutyPercent = 50.0f);
    Status off();

private:
    Pwm &pwm_;
    std::uint8_t channel_;
};

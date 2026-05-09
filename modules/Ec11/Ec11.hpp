#pragma once

#include <array>
#include <cstdint>

#include "Common.hpp"
#include "Gpio.hpp"

class Ec11 {
public:
    enum class EventType : std::uint8_t {
        Rotate = 0,
        Button = 1,
    };

    enum class ButtonEvent : std::uint8_t {
        None = 0,
        Click = 1,
        LongPress = 2,
    };

    struct Event {
        EventType type = EventType::Rotate;
        int movement = 0;
        ButtonEvent button = ButtonEvent::None;
        int cursor = 0;
    };

    Ec11(Gpio &clkPin, Gpio &dtPin, Gpio &swPin, int menuSize = 0);

    Status init();
    Status update();
    bool read(Event &event);
    int cursor() const { return cursor_; }
    void setMenuSize(int count) { menuSize_ = count; }
    void setInvert(bool invert) { invert_ = invert; }

private:
    void pushEvent(const Event &event);

    Gpio &clkPin_;
    Gpio &dtPin_;
    Gpio &swPin_;
    int menuSize_ = 0;
    int cursor_ = 0;
    bool invert_ = false;
    bool lastClk_ = false;
    bool lastSw_ = false;
    std::array<Event, 8> queue_{};
    std::size_t head_ = 0U;
    std::size_t tail_ = 0U;
};

#include "Ec11.hpp"

Ec11::Ec11(Gpio &clkPin, Gpio &dtPin, Gpio &swPin, int menuSize)
    : clkPin_(clkPin), dtPin_(dtPin), swPin_(swPin), menuSize_(menuSize)
{
}

Status Ec11::init()
{
    auto status = clkPin_.initInput();
    if (status != Status::Ok) {
        return status;
    }
    status = dtPin_.initInput();
    if (status != Status::Ok) {
        return status;
    }
    status = swPin_.initInput();
    if (status != Status::Ok) {
        return status;
    }
    lastClk_ = clkPin_.read();
    lastSw_ = swPin_.read();
    return Status::Ok;
}

Status Ec11::update()
{
    const bool clk = clkPin_.read();
    const bool dt = dtPin_.read();
    const bool sw = swPin_.read();

    if (clk != lastClk_ && clk) {
        const int movement = ((dt == clk) ? -1 : 1) * (invert_ ? -1 : 1);
        if (menuSize_ > 0) {
            cursor_ += movement;
            while (cursor_ < 0) {
                cursor_ += menuSize_;
            }
            cursor_ %= menuSize_;
        } else {
            cursor_ += movement;
        }
        pushEvent(Event{EventType::Rotate, movement, ButtonEvent::None, cursor_});
    }

    if (sw != lastSw_ && !sw) {
        pushEvent(Event{EventType::Button, 0, ButtonEvent::Click, cursor_});
    }

    lastClk_ = clk;
    lastSw_ = sw;
    return Status::Ok;
}

bool Ec11::read(Event &event)
{
    if (head_ == tail_) {
        return false;
    }
    event = queue_[tail_];
    tail_ = (tail_ + 1U) % queue_.size();
    return true;
}

void Ec11::pushEvent(const Event &event)
{
    queue_[head_] = event;
    head_ = (head_ + 1U) % queue_.size();
    if (head_ == tail_) {
        tail_ = (tail_ + 1U) % queue_.size();
    }
}

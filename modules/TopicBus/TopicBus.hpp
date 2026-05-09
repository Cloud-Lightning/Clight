#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include "Common.hpp"

template <std::size_t MaxSubscribers = 8>
class TopicBus {
public:
    using Callback = void(*)(std::string_view topic, ByteView payload, void *arg);

    Status subscribe(std::string_view topic, Callback callback, void *arg = nullptr)
    {
        if (topic.empty() || callback == nullptr) {
            return Status::Param;
        }

        for (auto &slot : subscribers_) {
            if (!slot.active) {
                slot.topic = topic;
                slot.callback = callback;
                slot.arg = arg;
                slot.active = true;
                return Status::Ok;
            }
        }

        return Status::Busy;
    }

    Status publish(std::string_view topic, ByteView payload) const
    {
        if (topic.empty()) {
            return Status::Param;
        }

        bool delivered = false;
        for (const auto &slot : subscribers_) {
            if (slot.active && slot.topic == topic) {
                slot.callback(topic, payload, slot.arg);
                delivered = true;
            }
        }

        return delivered ? Status::Ok : Status::Unsupported;
    }

private:
    struct Subscriber {
        std::string_view topic{};
        Callback callback = nullptr;
        void *arg = nullptr;
        bool active = false;
    };

    std::array<Subscriber, MaxSubscribers> subscribers_{};
};

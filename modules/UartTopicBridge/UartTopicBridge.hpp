#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "Common.hpp"
#include "TopicBus.hpp"
#include "Uart.hpp"

template <std::size_t Capacity>
class ByteRingBuffer {
public:
    std::size_t size() const { return size_; }
    std::size_t freeSpace() const { return Capacity - size_; }

    std::size_t push(ByteView data)
    {
        std::size_t written = 0U;
        for (auto byte : data) {
            if (size_ >= Capacity) {
                break;
            }
            buffer_[head_] = byte;
            head_ = (head_ + 1U) % Capacity;
            ++size_;
            ++written;
        }
        return written;
    }

    std::size_t pop(ByteSpan out)
    {
        std::size_t read = 0U;
        while ((read < out.size()) && (size_ > 0U)) {
            out[read++] = buffer_[tail_];
            tail_ = (tail_ + 1U) % Capacity;
            --size_;
        }
        return read;
    }

    bool popLine(ByteSpan out, std::size_t &line_size, std::uint8_t delimiter = '\n')
    {
        line_size = 0U;
        if (size_ == 0U) {
            return false;
        }

        std::size_t index = tail_;
        std::size_t count = 0U;
        bool found = false;
        while (count < size_) {
            if (buffer_[index] == delimiter) {
                found = true;
                break;
            }
            index = (index + 1U) % Capacity;
            ++count;
        }

        if (!found) {
            return false;
        }

        while ((line_size < out.size()) && (size_ > 0U)) {
            const auto byte = buffer_[tail_];
            tail_ = (tail_ + 1U) % Capacity;
            --size_;
            if (byte == delimiter) {
                break;
            }
            out[line_size++] = byte;
        }

        while (line_size > 0U && out[line_size - 1U] == '\r') {
            --line_size;
        }
        return true;
    }

private:
    std::array<std::uint8_t, Capacity> buffer_{};
    std::size_t head_ = 0U;
    std::size_t tail_ = 0U;
    std::size_t size_ = 0U;
};

template <std::size_t RingCapacity = 1024, std::size_t RxChunk = 128, std::size_t MaxSubscribers = 8>
class UartTopicBridge {
public:
    UartTopicBridge(Uart uart, TopicBus<MaxSubscribers> &bus, std::string_view default_topic = {})
        : uart_(uart), bus_(bus), default_topic_(default_topic)
    {
    }

    Status start(std::uint32_t baudrate = 0U,
                 TransferMode transferMode = TransferMode::Interrupt,
                 std::uint8_t irqPriority = DefaultIrqPriority)
    {
        auto status = uart_.init(baudrate,
                                 UartParity::None,
                                 UartStopBits::One,
                                 UartWordLength::Bits8,
                                 transferMode,
                                 irqPriority);
        if (status != Status::Ok) {
            return status;
        }

        status = uart_.onEvent(&UartTopicBridge::onEventStatic, this);
        if (status != Status::Ok) {
            return status;
        }

        started_ = true;
        return armReceive();
    }

    Status poll()
    {
        if (!started_) {
            return Status::Error;
        }

        const auto arm_status = armReceive();
        if (arm_status != Status::Ok && arm_status != Status::Busy && arm_status != Status::Timeout) {
            return arm_status;
        }

        if (pending_publish_) {
            publishPendingLines();
        }
        return Status::Ok;
    }

    Status publish(std::string_view topic, ByteView payload)
    {
        if (!started_) {
            return Status::Error;
        }
        if (topic.empty()) {
            return Status::Param;
        }

        const std::size_t total_size = topic.size() + 1U + payload.size() + 1U;
        if (total_size > tx_line_.size()) {
            return Status::Param;
        }

        std::size_t offset = 0U;
        for (const char ch : topic) {
            tx_line_[offset++] = static_cast<std::uint8_t>(ch);
        }
        tx_line_[offset++] = static_cast<std::uint8_t>(':');
        for (const auto byte : payload) {
            tx_line_[offset++] = byte;
        }
        tx_line_[offset++] = static_cast<std::uint8_t>('\n');
        return uart_.write(ByteView(tx_line_.data(), offset));
    }

    Status publish(std::string_view topic, std::string_view payload)
    {
        return publish(topic, ByteView(reinterpret_cast<const std::uint8_t *>(payload.data()), payload.size()));
    }

    Status publish(std::string_view topic)
    {
        return publish(topic, std::string_view{});
    }

    std::size_t available() const
    {
        return ring_.size();
    }

    std::size_t readRaw(ByteSpan out)
    {
        return ring_.pop(out);
    }

private:
    static void onEventStatic(bsp_uart_event_t event,
                              std::uint8_t *data,
                              std::uint16_t len,
                              std::int32_t status,
                              void *arg)
    {
        auto *self = static_cast<UartTopicBridge *>(arg);
        if (self != nullptr && status == BSP_OK &&
            (event == BSP_UART_EVENT_RX_READY || event == BSP_UART_EVENT_RX_DONE || event == BSP_UART_EVENT_RX_IDLE)) {
            self->onReceive(ByteView(data, len));
        }
    }

    void onReceive(ByteView chunk)
    {
        if (ring_.push(chunk) != chunk.size()) {
            overflowed_ = true;
        }
        pending_publish_ = true;
    }

    Status armReceive()
    {
        return uart_.readAsync(ByteSpan(rx_chunk_.data(), rx_chunk_.size()));
    }

    void publishPendingLines()
    {
        std::size_t line_size = 0U;
        while (ring_.popLine(ByteSpan(line_buffer_.data(), line_buffer_.size()), line_size)) {
            const std::string_view line(reinterpret_cast<const char *>(line_buffer_.data()), line_size);
            if (line.empty()) {
                continue;
            }

            const auto delimiter = line.find(':');
            if (delimiter == std::string_view::npos) {
                if (!default_topic_.empty()) {
                    (void) bus_.publish(default_topic_, ByteView(line_buffer_.data(), line_size));
                }
                continue;
            }

            const auto topic = line.substr(0U, delimiter);
            const auto payload = ByteView(line_buffer_.data() + delimiter + 1U, line_size - delimiter - 1U);
            if (!topic.empty()) {
                (void) bus_.publish(topic, payload);
            }
        }
        pending_publish_ = false;
    }

    Uart uart_;
    TopicBus<MaxSubscribers> &bus_;
    std::string_view default_topic_;
    bool started_ = false;
    bool pending_publish_ = false;
    bool overflowed_ = false;
    ByteRingBuffer<RingCapacity> ring_{};
    std::array<std::uint8_t, RxChunk> rx_chunk_{};
    std::array<std::uint8_t, RingCapacity> line_buffer_{};
    std::array<std::uint8_t, RingCapacity + RxChunk + 32U> tx_line_{};
};

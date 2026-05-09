#pragma once

#include <cstdint>

#include "Common.hpp"

struct BleAdvertisingConfig {
    const char *deviceName = "Clight ESP32";
    std::uint16_t appearance = 0U;
};

class BleService {
public:
    Status init(const char *deviceName = "Clight ESP32");
    Status startAdvertising(const BleAdvertisingConfig &config = {});
    Status addGattService(std::uint16_t uuid16);
    bool available() const;
};

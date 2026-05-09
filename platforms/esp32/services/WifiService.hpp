#pragma once

#include <cstdint>

#include "Common.hpp"

struct WifiStaConfig {
    const char *ssid = "";
    const char *password = "";
    std::uint32_t timeoutMs = 10000U;
    bool autoReconnect = true;
};

struct WifiApConfig {
    const char *ssid = "Clight-ESP32";
    const char *password = "";
    std::uint8_t channel = 6U;
    std::uint8_t maxConnections = 4U;
    bool hidden = false;
};

struct WifiStatus {
    bool initialized = false;
    bool started = false;
    bool staEnabled = false;
    bool apEnabled = false;
    bool staConnected = false;
    char ip[16] = "0.0.0.0";
    std::int8_t rssi = 0;
};

class WifiService {
public:
    Status init();
    Status connectSta(const WifiStaConfig &config);
    Status startAp(const WifiApConfig &config = {});
    Status startStaAp(const WifiStaConfig &staConfig, const WifiApConfig &apConfig = {});
    Status stop();
    WifiStatus status() const;

private:
    Status applyMode(bool sta, bool ap);
};

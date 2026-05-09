#include "WifiService.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "bsp/interface/bsp_wifi.h"

extern "C" {
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
}

namespace {

constexpr EventBits_t kWifiConnectedBit = BIT0;
constexpr EventBits_t kWifiFailedBit = BIT1;
constexpr int kMaxStaRetry = 5;

EventGroupHandle_t s_wifi_events = nullptr;
WifiStatus s_status{};
int s_retry_count = 0;
bool s_handlers_registered = false;
bool s_sta_netif_created = false;
bool s_ap_netif_created = false;
bool s_wifi_started = false;

void copy_string(std::uint8_t *dst, std::size_t dst_size, const char *src)
{
    if ((dst == nullptr) || (dst_size == 0U)) {
        return;
    }
    std::memset(dst, 0, dst_size);
    if (src != nullptr) {
        const auto len = std::min<std::size_t>(std::strlen(src), dst_size - 1U);
        std::memcpy(dst, src, len);
    }
}

void ip_to_string(const esp_ip4_addr_t &ip, char (&out)[16])
{
    (void)snprintf(out, sizeof(out), IPSTR, IP2STR(&ip));
}

void wifi_event_handler(void *, esp_event_base_t event_base, std::int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_STA_START) {
            s_status.staEnabled = true;
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            s_status.staConnected = false;
            std::strcpy(s_status.ip, "0.0.0.0");
            if (s_retry_count < kMaxStaRetry) {
                ++s_retry_count;
                (void)esp_wifi_connect();
            } else if (s_wifi_events != nullptr) {
                xEventGroupSetBits(s_wifi_events, kWifiFailedBit);
            }
        } else if (event_id == WIFI_EVENT_AP_START) {
            s_status.apEnabled = true;
        } else if (event_id == WIFI_EVENT_AP_STOP) {
            s_status.apEnabled = false;
        }
    } else if ((event_base == IP_EVENT) && (event_id == IP_EVENT_STA_GOT_IP)) {
        const auto *event = static_cast<ip_event_got_ip_t *>(event_data);
        s_retry_count = 0;
        s_status.staConnected = true;
        ip_to_string(event->ip_info.ip, s_status.ip);
        if (s_wifi_events != nullptr) {
            xEventGroupSetBits(s_wifi_events, kWifiConnectedBit);
        }
    }
}

Status register_handlers()
{
    if (s_handlers_registered) {
        return Status::Ok;
    }

    if (esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, nullptr, nullptr) != ESP_OK) {
        return Status::Error;
    }
    if (esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, nullptr, nullptr) != ESP_OK) {
        return Status::Error;
    }

    s_handlers_registered = true;
    return Status::Ok;
}

wifi_mode_t mode_from_flags(bool sta, bool ap)
{
    if (sta && ap) {
        return WIFI_MODE_APSTA;
    }
    if (ap) {
        return WIFI_MODE_AP;
    }
    return WIFI_MODE_STA;
}

} // namespace

Status WifiService::init()
{
    if (s_status.initialized) {
        return Status::Ok;
    }

    if (bsp_wifi_init(BSP_WIFI_MAIN) != BSP_OK) {
        return Status::Error;
    }

    if (s_wifi_events == nullptr) {
        s_wifi_events = xEventGroupCreate();
        if (s_wifi_events == nullptr) {
            return Status::Error;
        }
    }

    if (!s_sta_netif_created) {
        esp_netif_create_default_wifi_sta();
        s_sta_netif_created = true;
    }
    if (!s_ap_netif_created) {
        esp_netif_create_default_wifi_ap();
        s_ap_netif_created = true;
    }

    const auto handler_status = register_handlers();
    if (handler_status != Status::Ok) {
        return handler_status;
    }

    s_status.initialized = true;
    return Status::Ok;
}

Status WifiService::applyMode(bool sta, bool ap)
{
    if (!sta && !ap) {
        return Status::Param;
    }

    const auto init_status = init();
    if (init_status != Status::Ok) {
        return init_status;
    }

    if (s_wifi_started) {
        if (esp_wifi_stop() != ESP_OK) {
            return Status::Error;
        }
        s_wifi_started = false;
        s_status.started = false;
    }

    if (esp_wifi_set_mode(mode_from_flags(sta, ap)) != ESP_OK) {
        return Status::Error;
    }

    s_status.staEnabled = sta;
    s_status.apEnabled = ap;
    return Status::Ok;
}

Status WifiService::connectSta(const WifiStaConfig &config)
{
    if ((config.ssid == nullptr) || (config.ssid[0] == '\0')) {
        return Status::Param;
    }

    const auto mode_status = applyMode(true, s_status.apEnabled);
    if (mode_status != Status::Ok) {
        return mode_status;
    }

    wifi_config_t wifi_config = {};
    copy_string(wifi_config.sta.ssid, sizeof(wifi_config.sta.ssid), config.ssid);
    copy_string(wifi_config.sta.password, sizeof(wifi_config.sta.password), config.password);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
    wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

    if ((config.password != nullptr) && (config.password[0] != '\0')) {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    }

    if (esp_wifi_set_config(WIFI_IF_STA, &wifi_config) != ESP_OK) {
        return Status::Error;
    }

    if (s_wifi_events != nullptr) {
        xEventGroupClearBits(s_wifi_events, kWifiConnectedBit | kWifiFailedBit);
    }
    s_retry_count = config.autoReconnect ? 0 : kMaxStaRetry;

    if (esp_wifi_start() != ESP_OK) {
        return Status::Error;
    }
    s_wifi_started = true;
    s_status.started = true;

    if (esp_wifi_connect() != ESP_OK) {
        return Status::Error;
    }

    if ((config.timeoutMs != 0U) && (s_wifi_events != nullptr)) {
        const EventBits_t bits = xEventGroupWaitBits(
            s_wifi_events,
            kWifiConnectedBit | kWifiFailedBit,
            pdFALSE,
            pdFALSE,
            pdMS_TO_TICKS(config.timeoutMs));
        if ((bits & kWifiConnectedBit) != 0U) {
            return Status::Ok;
        }
        if ((bits & kWifiFailedBit) != 0U) {
            return Status::Error;
        }
        return Status::Timeout;
    }

    return Status::Ok;
}

Status WifiService::startAp(const WifiApConfig &config)
{
    if ((config.ssid == nullptr) || (config.ssid[0] == '\0') || (config.maxConnections == 0U)) {
        return Status::Param;
    }

    const auto mode_status = applyMode(s_status.staEnabled, true);
    if (mode_status != Status::Ok) {
        return mode_status;
    }

    wifi_config_t wifi_config = {};
    copy_string(wifi_config.ap.ssid, sizeof(wifi_config.ap.ssid), config.ssid);
    copy_string(wifi_config.ap.password, sizeof(wifi_config.ap.password), config.password);
    wifi_config.ap.ssid_len = static_cast<std::uint8_t>(std::strlen(config.ssid));
    wifi_config.ap.channel = config.channel;
    wifi_config.ap.max_connection = config.maxConnections;
    wifi_config.ap.ssid_hidden = config.hidden ? 1U : 0U;
    wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    if ((config.password != nullptr) && (std::strlen(config.password) >= 8U)) {
        wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    }

    if (esp_wifi_set_config(WIFI_IF_AP, &wifi_config) != ESP_OK) {
        return Status::Error;
    }

    if (esp_wifi_start() != ESP_OK) {
        return Status::Error;
    }

    s_wifi_started = true;
    s_status.started = true;
    s_status.apEnabled = true;
    return Status::Ok;
}

Status WifiService::startStaAp(const WifiStaConfig &staConfig, const WifiApConfig &apConfig)
{
    const auto ap_status = startAp(apConfig);
    if (ap_status != Status::Ok) {
        return ap_status;
    }
    return connectSta(staConfig);
}

Status WifiService::stop()
{
    if (!s_status.initialized || !s_wifi_started) {
        return Status::Ok;
    }

    if (esp_wifi_stop() != ESP_OK) {
        return Status::Error;
    }

    s_wifi_started = false;
    s_status.started = false;
    s_status.staConnected = false;
    s_status.staEnabled = false;
    s_status.apEnabled = false;
    std::strcpy(s_status.ip, "0.0.0.0");
    return Status::Ok;
}

WifiStatus WifiService::status() const
{
    WifiStatus result = s_status;
    if (result.staConnected) {
        wifi_ap_record_t ap_info = {};
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            result.rssi = ap_info.rssi;
        }
    }
    return result;
}

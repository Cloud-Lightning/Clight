#include "HttpServerService.hpp"

#include <cstdio>
#include <cstring>

extern "C" {
#include "driver/gpio.h"
#include "esp_timer.h"
}

namespace {

constexpr const char *kIndexHtml =
    "<!doctype html><html><head><meta charset=\"utf-8\">"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<title>Clight ESP32</title>"
    "<style>body{margin:0;background:#10151c;color:#f6f2df;font-family:Arial,sans-serif;}"
    "main{max-width:760px;margin:48px auto;padding:24px;border:1px solid #344252;background:#17202b;}"
    "h1{letter-spacing:.08em;color:#40e0ff}.card{margin:16px 0;padding:16px;background:#0b1118}"
    "button{padding:12px 18px;background:#ffcf40;border:0;font-weight:700}</style></head>"
    "<body><main><h1>CLIGHT ESP32 GATEWAY</h1>"
    "<div class=\"card\">WiFi/AP + HTTP service is running.</div>"
    "<button onclick=\"fetch('/api/led',{method:'POST'}).then(()=>fetch('/api/status')).then(r=>r.text()).then(alert)\">TOGGLE LED</button>"
    "</main></body></html>";

esp_err_t index_handler(httpd_req_t *request)
{
    httpd_resp_set_type(request, "text/html");
    return httpd_resp_send(request, kIndexHtml, HTTPD_RESP_USE_STRLEN);
}

esp_err_t status_handler(httpd_req_t *request)
{
    char response[160] = {};
    const auto uptime_ms = static_cast<long long>(esp_timer_get_time() / 1000);
    std::snprintf(response, sizeof(response),
                  "{\"name\":\"Clight ESP32\",\"uptime_ms\":%lld,\"http\":\"ok\"}",
                  uptime_ms);
    httpd_resp_set_type(request, "application/json");
    return httpd_resp_send(request, response, HTTPD_RESP_USE_STRLEN);
}

esp_err_t led_handler(httpd_req_t *request)
{
    static bool initialized = false;
    static bool state = false;
    if (!initialized) {
        gpio_reset_pin(GPIO_NUM_2);
        gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
        initialized = true;
    }
    state = !state;
    gpio_set_level(GPIO_NUM_2, state ? 1 : 0);
    httpd_resp_set_type(request, "application/json");
    return httpd_resp_sendstr(request, state ? "{\"led\":true}" : "{\"led\":false}");
}

Status register_uri(httpd_handle_t server, const char *uri, httpd_method_t method, HttpServerService::Handler handler)
{
    if ((server == nullptr) || (uri == nullptr) || (handler == nullptr)) {
        return Status::Param;
    }

    httpd_uri_t entry = {};
    entry.uri = uri;
    entry.method = method;
    entry.handler = handler;
    entry.user_ctx = nullptr;
    return (httpd_register_uri_handler(server, &entry) == ESP_OK) ? Status::Ok : Status::Error;
}

} // namespace

Status HttpServerService::start(std::uint16_t port)
{
    if (server_ != nullptr) {
        return Status::Ok;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;
    config.ctrl_port = static_cast<std::uint16_t>(port + 1U);
    config.lru_purge_enable = true;

    if (httpd_start(&server_, &config) != ESP_OK) {
        server_ = nullptr;
        return Status::Error;
    }

    const auto root_status = registerGet("/", index_handler);
    const auto status_status = registerGet("/api/status", status_handler);
    const auto led_status = registerPost("/api/led", led_handler);
    if ((root_status != Status::Ok) || (status_status != Status::Ok) || (led_status != Status::Ok)) {
        (void)stop();
        return Status::Error;
    }

    return Status::Ok;
}

Status HttpServerService::stop()
{
    if (server_ == nullptr) {
        return Status::Ok;
    }

    const auto status = httpd_stop(server_);
    server_ = nullptr;
    return (status == ESP_OK) ? Status::Ok : Status::Error;
}

Status HttpServerService::registerGet(const char *uri, Handler handler)
{
    return register_uri(server_, uri, HTTP_GET, handler);
}

Status HttpServerService::registerPost(const char *uri, Handler handler)
{
    return register_uri(server_, uri, HTTP_POST, handler);
}

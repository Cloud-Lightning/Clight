#pragma once

#include <cstdint>

#include "Common.hpp"

extern "C" {
#include "esp_http_server.h"
}

class HttpServerService {
public:
    using Handler = esp_err_t (*)(httpd_req_t *request);

    Status start(std::uint16_t port = 80U);
    Status stop();
    bool running() const { return server_ != nullptr; }

    Status registerGet(const char *uri, Handler handler);
    Status registerPost(const char *uri, Handler handler);

private:
    httpd_handle_t server_ = nullptr;
};

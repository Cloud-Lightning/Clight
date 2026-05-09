#include "NvsService.hpp"

extern "C" {
#include "esp_err.h"
#include "nvs_flash.h"
}

Status NvsService::init() const
{
    esp_err_t err = nvs_flash_init();
    if ((err == ESP_ERR_NVS_NO_FREE_PAGES) || (err == ESP_ERR_NVS_NEW_VERSION_FOUND)) {
        if (nvs_flash_erase() != ESP_OK) {
            return Status::Error;
        }
        err = nvs_flash_init();
    }

    return (err == ESP_OK) ? Status::Ok : Status::Error;
}

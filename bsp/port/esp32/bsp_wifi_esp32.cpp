#include "bsp_wifi.h"

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

static bool s_wifi_initialized = false;

static int32_t ensure_nvs()
{
    esp_err_t err = nvs_flash_init();
    if ((err == ESP_ERR_NVS_NO_FREE_PAGES) || (err == ESP_ERR_NVS_NEW_VERSION_FOUND)) {
        if (nvs_flash_erase() != ESP_OK) {
            return BSP_ERROR;
        }
        err = nvs_flash_init();
    }
    return (err == ESP_OK) ? BSP_OK : BSP_ERROR;
}

int32_t bsp_wifi_init(bsp_wifi_instance_t instance)
{
    if (instance >= BSP_WIFI_MAX) {
        return BSP_ERROR_PARAM;
    }

    if (s_wifi_initialized) {
        return BSP_OK;
    }

    if (ensure_nvs() != BSP_OK) {
        return BSP_ERROR;
    }

    esp_err_t err = esp_netif_init();
    if ((err != ESP_OK) && (err != ESP_ERR_INVALID_STATE)) {
        return BSP_ERROR;
    }

    err = esp_event_loop_create_default();
    if ((err != ESP_OK) && (err != ESP_ERR_INVALID_STATE)) {
        return BSP_ERROR;
    }

    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&config);
    if ((err != ESP_OK) && (err != ESP_ERR_WIFI_INIT_STATE)) {
        return BSP_ERROR;
    }

    if (esp_wifi_set_storage(WIFI_STORAGE_RAM) != ESP_OK) {
        return BSP_ERROR;
    }

    s_wifi_initialized = true;
    return BSP_OK;
}

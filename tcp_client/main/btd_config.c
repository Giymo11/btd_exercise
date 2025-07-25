#include <stdio.h>
#include <string.h>

#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#include "btd_config.h"

#define NVS_NAMESPACE "btd_cfg"
#define NVS_KEY "config"

static const char *TAG = "BTD_CONFIG";

esp_err_t btd_read_config(btd_config_t *config)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);

    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        nvs_close(handle);
        ESP_LOGI(TAG, "No configuration found, using defaults.");
        // Initialize config with default values
        memcpy(config, &DEFAULT_CONFIG, sizeof(btd_config_t));
        return btd_save_config(config);
    }
    ESP_ERROR_CHECK(err);

    size_t required_size = sizeof(btd_config_t);
    err = nvs_get_blob(handle, NVS_KEY, config, &required_size);
    nvs_close(handle);
    return err;
}

esp_err_t btd_save_config(const btd_config_t *config)
{
    nvs_handle_t handle;

    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK)
        return err;

    err = nvs_set_blob(handle, NVS_KEY, config, sizeof(btd_config_t));
    if (err == ESP_OK)
        err = nvs_commit(handle);

    nvs_close(handle);
    return err;
}

esp_err_t btd_delete_config(void)
{
    ESP_ERROR_CHECK(nvs_flash_erase()); // Erase NVS to reset configuration
    return nvs_flash_init(); // Reinitialize NVS
}


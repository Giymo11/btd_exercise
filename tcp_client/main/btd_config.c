#include <stdio.h>
#include <string.h>

#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#include "btd_config.h"

#define CONFIG_NAMESPACE "btd_cfg"
#define CONFIG_KEY "config"

static const char *TAG = "BTD_CONFIG";

esp_err_t btd_read_config(btd_config_t *config)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(CONFIG_NAMESPACE, NVS_READONLY, &handle);

    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        nvs_close(handle);
        ESP_LOGI(TAG, "No configuration found, using defaults.");
        config->workTimeSeconds = 25 * 60;
        config->breakTimeSeconds = 5 * 60;
        config->longBreakTimeSeconds = 15 * 60;
        config->longBreakSessionCount = 4;
        strcpy(config->wifiPw, "default_password");
        config->breakGestureEnabled = true;
        return btd_save_config(config);
    }
    ESP_ERROR_CHECK(err);

    size_t required_size = sizeof(btd_config_t);
    err = nvs_get_blob(handle, CONFIG_KEY, config, &required_size);
    nvs_close(handle);
    return err;
}

esp_err_t btd_save_config(const btd_config_t *config)
{
    nvs_handle_t handle;

    esp_err_t err = nvs_open(CONFIG_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK)
        return err;

    err = nvs_set_blob(handle, CONFIG_KEY, config, sizeof(btd_config_t));
    if (err == ESP_OK)
        err = nvs_commit(handle);

    nvs_close(handle);
    return err;
}



#include "freertos/FreeRTOS.h" // FreeRTOS API
#include "freertos/task.h"     // Task management
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_system.h"
#include "btd_stats.h"

static const char *TAG = "BTD_STATS";

static const char *NVS_NAMESPACE = "btd_stats";
static const char *NVS_KEY_SESSION_COUNT = "session_count";
static const char *NVS_KEY_SESSION_PREFIX = "session_";
static const char *NVS_KEY_SESSION_ID = "session_id";

static int session_id = -1;

esp_err_t record_work_session(session_stats_t *stats)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }

    // if not set, read and increment session_id
    if (session_id < 0)
    {
        size_t required_size = sizeof(session_id);
        err = nvs_get_u32(nvs_handle, NVS_KEY_SESSION_ID, (uint32_t *)&session_id);
        if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            session_id = 0; // Start from 0 if not found
        }
        else if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error reading session ID: %s", esp_err_to_name(err));
            goto cleanup;
        }
        session_id++;
        err = nvs_set_u32(nvs_handle, NVS_KEY_SESSION_ID, session_id);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error saving session ID: %s", esp_err_to_name(err));
            goto cleanup;
        }
    }
    // Save the session stats with the current session ID
    stats->session_id = session_id; 

    uint32_t count = 0;
    err = nvs_get_u32(nvs_handle, NVS_KEY_SESSION_COUNT, &count);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        count = 0; // No sessions recorded yet
    }
    else if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error reading session count: %s", esp_err_to_name(err));
        goto cleanup;
    }

    char key[16];
    snprintf(key, sizeof(key), "session_%lu", count);
    err = nvs_set_blob(nvs_handle, key, stats, sizeof(session_stats_t));
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error saving session stats: %s", esp_err_to_name(err));
        goto cleanup;
    }

    count++;
    err = nvs_set_u32(nvs_handle, NVS_KEY_SESSION_COUNT, count);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error updating session count: %s", esp_err_to_name(err));
    }

cleanup:
    nvs_close(nvs_handle);
    return err;
}

esp_err_t get_all_work_sessions(session_stats_t *sessions, size_t *count)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }

    uint32_t session_count = 0;
    err = nvs_get_u32(nvs_handle, NVS_KEY_SESSION_COUNT, &session_count);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        session_count = 0; // No sessions recorded yet
    }
    else if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error reading session count: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    for (size_t i = 0; i < session_count && i < *count; i++)
    {
        char key[32];
        snprintf(key, sizeof(key), "session_%zu", i);
        size_t required_size = sizeof(session_stats_t);
        err = nvs_get_blob(nvs_handle, key, &sessions[i], &required_size);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error reading session stats for index %zu: %s", i, esp_err_to_name(err));
            break; // Stop on error
        }
    }

    *count = session_count; // Return the actual number of sessions read
    nvs_close(nvs_handle);
    return ESP_OK;
}

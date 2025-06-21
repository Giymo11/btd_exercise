#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h" // FreeRTOS API
#include "freertos/task.h"     // Task management
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "math.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "btd_wifi.h"

static const char *TAG = "BTD_WIFI";

#define NVS_NAMESPACE "btd_wifi"
#define NVS_KEY_FPCOUNT "fp_count"
#define NVS_FP_PREFIX "fp_"

esp_err_t start_wifi_ap(const char *ssid, const char *password)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

    wifi_config_t ap_config = {0};
    strncpy((char *)ap_config.ap.ssid, ssid, sizeof(ap_config.ap.ssid) - 1);
    strncpy((char *)ap_config.ap.password, password, sizeof(ap_config.ap.password) - 1);
    ap_config.ap.ssid_len = strlen(ssid);
    ap_config.ap.channel = 1;
    ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    ap_config.ap.max_connection = 4;

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    return esp_wifi_start();
}

esp_err_t stop_wifi_ap()
{
    ESP_ERROR_CHECK(esp_wifi_stop());
    return esp_wifi_deinit();
}

static void make_fingerprint_key(char *key, size_t key_size, uint8_t index)
{
    snprintf(key, key_size, "%s%02d", NVS_FP_PREFIX, index);
}

static void add_location_name(wifi_location_fingerprint_t *fp)
{
    if (fp->ap_count == 0)
        snprintf(fp->name, LOCATION_NAME_MAX_LEN, "Unknown_Location");
    else if (strlen(fp->aps[0].ssid) == 0)
        snprintf(fp->name, LOCATION_NAME_MAX_LEN, "Hidden_Location");
    else
        snprintf(fp->name, LOCATION_NAME_MAX_LEN, "%.24s_%04X", fp->aps[0].ssid, fp->aps[0].bssid[5]);
}

static int compare_wifi_records(const void *a, const void *b)
{
    wifi_ap_record_t *rec_a = (wifi_ap_record_t *)a;
    wifi_ap_record_t *rec_b = (wifi_ap_record_t *)b;
    return rec_b->rssi - rec_a->rssi; // Sort descending
}

static double compare_fingerprints(const wifi_location_fingerprint_t *fp1, const wifi_location_fingerprint_t *fp2)
{
    int common_aps = 0;
    double rssi_squared_delta = 0;

    for (int i = 0; i < fp1->ap_count; i++)
    {
        for (int j = 0; j < fp2->ap_count; j++)
        {
            if (memcmp(fp1->aps[i].bssid, fp2->aps[j].bssid, BSSID_LEN) == 0)
            {
                common_aps++;
                int rssi_delta = fp1->aps[i].rssi - fp2->aps[j].rssi;
                rssi_squared_delta += rssi_delta * rssi_delta; // Squared difference for similarity
                break;
            }
        }
    }

    if (common_aps == 0)
        return 0;
    double common_ratio = (double)common_aps / FINGERPRINT_MAX_APS;
    double rssi_similarity = 1.0 - (rssi_squared_delta / (common_aps * 100 * 100)); // Normalize RSSI delta
    return (0.7 * common_ratio) + (0.3 * rssi_similarity);
}

static void fill_fingerprint_from_scan(wifi_location_fingerprint_t *fp, wifi_ap_record_t *ap_info, uint16_t ap_found_count, uint16_t scan_list_size)
{
    // Sort APs by RSSI
    qsort(ap_info, ap_found_count, sizeof(wifi_ap_record_t), compare_wifi_records);
    for (int i = 0; i < ap_found_count && i < scan_list_size; i++)
    {
        ESP_LOGI(TAG, "SSID: %s, RSSI: %d", ap_info[i].ssid, ap_info[i].rssi);
    }

    // Fill the fingerprint with the top FINGERPRINT_MAX_APS strongest APs
    fp->ap_count = (ap_found_count < FINGERPRINT_MAX_APS) ? ap_found_count : FINGERPRINT_MAX_APS;
    for (int i = 0; i < fp->ap_count; i++)
    {
        memcpy(fp->aps[i].bssid, ap_info[i].bssid, BSSID_LEN);
        strncpy(fp->aps[i].ssid, (char *)ap_info[i].ssid, SSID_MAX_LEN - 1);
        fp->aps[i].ssid[SSID_MAX_LEN - 1] = '\0'; // Ensure null termination
        fp->aps[i].rssi = ap_info[i].rssi;
    }
}

static esp_err_t scan_and_create_fingerprint(wifi_location_fingerprint_t *fp)
{
    fp->ap_count = 0;

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    wifi_scan_config_t scan_config = {.show_hidden = true, .scan_type = WIFI_SCAN_TYPE_ACTIVE};
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));

    uint16_t ap_found_count = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_found_count));

    if (ap_found_count != 0)
    {
        uint16_t max_ap_records = ap_found_count;
        wifi_ap_record_t *ap_info = malloc(max_ap_records * sizeof(wifi_ap_record_t));
        if (ap_info == NULL)
        {
            ESP_LOGE(TAG, "Failed to allocate memory for AP records");
            return ESP_ERR_NO_MEM;
        }
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_found_count, ap_info));
        ESP_LOGI(TAG, "Total APs scanned = %u, actual AP number ap_info holds = %u", ap_found_count, max_ap_records);

        fill_fingerprint_from_scan(fp, ap_info, ap_found_count, max_ap_records);
        free(ap_info);
    }

    add_location_name(fp);
    ESP_LOGI(TAG, "Location fingerprint created: %s", fp->name);

    ESP_ERROR_CHECK(stop_wifi_ap());
    return ESP_OK;
}

static uint8_t get_fingerprint_count(nvs_handle_t nvs_handle)
{
    uint8_t fp_count = 0;
    esp_err_t err = nvs_get_u8(nvs_handle, NVS_KEY_FPCOUNT, &fp_count);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        fp_count = 0; // No fingerprints stored yet
    }
    else if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error reading fingerprint count: %s", esp_err_to_name(err));
        fp_count = 0; // Reset on error
    }
    return fp_count;
}

esp_err_t get_best_fingerprint_match(const nvs_handle_t nvs_handle, const uint8_t fp_count, wifi_location_fingerprint_t *fp, int *best_match_index, double *best_similarity, char *location_name_buffer, size_t buffer_size)
{
    *best_match_index = -1;
    *best_similarity = 0.0;

    for (uint8_t i = 0; i < fp_count; i++)
    {
        char key[16];
        make_fingerprint_key(key, sizeof(key), i);

        wifi_location_fingerprint_t existing_fp;
        size_t required_size = sizeof(existing_fp);
        esp_err_t err = nvs_get_blob(nvs_handle, key, &existing_fp, &required_size);
        if (err != ESP_OK)
            continue; // Skip if error reading fingerprint

        double similarity = compare_fingerprints(fp, &existing_fp);
        if (similarity > *best_similarity)
        {
            *best_similarity = similarity;
            *best_match_index = i;
            snprintf(location_name_buffer, buffer_size, "%s", existing_fp.name);
        }
    }

    return ESP_OK;
}

static esp_err_t save_fingerprint_to_nvs(const wifi_location_fingerprint_t *fp, uint8_t index)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
        return err;

    char key[16];
    make_fingerprint_key(key, sizeof(key), index);
    err = nvs_set_u8(nvs_handle, NVS_KEY_FPCOUNT, index + 1); // Store the count of fingerprints
    err = nvs_set_blob(nvs_handle, key, fp, sizeof(wifi_location_fingerprint_t));
    if (err == ESP_OK)
    {
        err = nvs_commit(nvs_handle);
    }
    nvs_close(nvs_handle);
    return err;
}

esp_err_t get_wifi_location_fingerprint(char *location_name_buffer, size_t buffer_size)
{
    wifi_location_fingerprint_t fp = {0};
    ESP_ERROR_CHECK(scan_and_create_fingerprint(&fp));

    nvs_handle_t nvs_handle;
    ESP_ERROR_CHECK(nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle));

    uint8_t fp_count = get_fingerprint_count(nvs_handle);
    ESP_LOGI(TAG, "Current fingerprint count: %d", fp_count);

    if (fp_count > 0)
    {
        int best_match_index = -1;
        double best_similarity = 0.0;

        ESP_ERROR_CHECK(get_best_fingerprint_match(
            nvs_handle,
            fp_count,
            &fp,
            &best_match_index,
            &best_similarity,
            location_name_buffer,
            buffer_size));

        if (best_match_index != -1 && best_similarity > 0.5) // Threshold for considering a match
        {
            ESP_LOGI(TAG, "Best match found: %s with similarity %.2f", location_name_buffer, best_similarity);
            nvs_close(nvs_handle);
            return ESP_OK; // Best match found
        }
    }

    ESP_LOGI(TAG, "No good match found, creating new fingerprint.");
    snprintf(location_name_buffer, buffer_size, "%s", fp.name);

    if (fp_count < FINGERPRINT_MAX_LOCATIONS)
    {
        ESP_LOGI(TAG, "Saving new fingerprint.");
        esp_err_t err = save_fingerprint_to_nvs(&fp, fp_count);
        nvs_close(nvs_handle);
        return err; // Error saving fingerprint
    }
    
    ESP_LOGW(TAG, "Maximum number of fingerprints reached (%d). Cannot save new fingerprint.", FINGERPRINT_MAX_LOCATIONS);
    nvs_close(nvs_handle);
    return ESP_OK;
}
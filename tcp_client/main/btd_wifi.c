#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h" // FreeRTOS API
#include "freertos/task.h"     // Task management
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "math.h"


#include "btd_wifi.h"

static const char *TAG = "BTD_WIFI";


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



static int compare_wifi_records(const void *a, const void *b)
{
    wifi_ap_record_t *rec_a = (wifi_ap_record_t *)a;
    wifi_ap_record_t *rec_b = (wifi_ap_record_t *)b;
    return rec_b->rssi - rec_a->rssi; // Sort descending
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
    wifi_ap_record_t ap_info[SCAN_LIST_SIZE];

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    wifi_scan_config_t scan_config = {.show_hidden = true, .scan_type = WIFI_SCAN_TYPE_ACTIVE};
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));

    uint16_t ap_found_count = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_found_count));
    uint16_t max_ap_records = SCAN_LIST_SIZE;
    if (ap_found_count != 0)
    {
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_found_count, ap_info));
        ESP_LOGI(TAG, "Total APs scanned = %u, actual AP number ap_info holds = %u", ap_found_count, max_ap_records);
        
        fill_fingerprint_from_scan(fp, ap_info, ap_found_count, max_ap_records);
    }

    add_location_name(fp);
    ESP_LOGI(TAG, "Location fingerprint created: %s", fp->name);

    ESP_ERROR_CHECK(stop_wifi_ap());
    return ESP_OK;
}

static double compare_fingerprints(const wifi_location_fingerprint_t *fp1, const wifi_location_fingerprint_t *fp2) {
    int common_aps = 0;
    double rssi_squared_delta = 0;

    for (int i = 0; i < fp1->ap_count; i++) {
        for (int j = 0; j < fp2->ap_count; j++) {
            if (memcmp(fp1->aps[i].bssid, fp2->aps[j].bssid, BSSID_LEN) == 0) {
                common_aps++;
                int rssi_delta = fp1->aps[i].rssi - fp2->aps[j].rssi;
                rssi_squared_delta += rssi_delta * rssi_delta; // Squared difference for similarity
                break;
            }
        }
    }

    if (common_aps == 0) return 0;
    double common_ratio = (double)common_aps / FINGERPRINT_MAX_APS;
    double rssi_similarity = 1.0 - (rssi_squared_delta / (common_aps * 100 * 100)); // Normalize RSSI delta
    return (0.7 * common_ratio) + (0.3 * rssi_similarity);
}

esp_err_t get_wifi_location_fingerprint(char *location_name_buffer, size_t buffer_size) {
    wifi_location_fingerprint_t fp = {0};
    ESP_ERROR_CHECK(scan_and_create_fingerprint(&fp));

    // TODO: Implement logic to compare with existing fingerprints

    snprintf(location_name_buffer, buffer_size, "%s", fp.name);
    return ESP_OK;
}
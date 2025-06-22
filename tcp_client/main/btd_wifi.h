
#ifndef WIFI_FINGERPRINT_H
#define WIFI_FINGERPRINT_H

#define FINGERPRINT_MAX_APS 3       
#define FINGERPRINT_MAX_LOCATIONS 10 // Max number of locations we can store
#define LOCATION_NAME_MAX_LEN 32     // Max length for a location name
#define SCAN_LIST_SIZE 10 // Max number of APs to scan for

#define BSSID_LEN 6     // Length of BSSID in bytes
#define SSID_MAX_LEN 33 // Max length of SSID (with null terminator)

typedef struct
{
    uint8_t bssid[BSSID_LEN];
    char ssid[SSID_MAX_LEN];
    int8_t rssi; // Signal strength
} fingerprint_ap_t;

typedef struct
{
    char name[LOCATION_NAME_MAX_LEN];
    fingerprint_ap_t aps[FINGERPRINT_MAX_APS];
    uint8_t ap_count; // Actual number of APs stored (can be less than 3)
} wifi_location_fingerprint_t;

/* * @brief Creates a Wi-Fi location fingerprint by scanning for nearby access points.
 * 
 * This function scans for Wi-Fi access points and creates a location name based on the strongest signals.
 * OUTPUT:
 * - location_name_buffer: Buffer to store the location name.
 * INPUT:
 * - buffer_size: Size of the location name buffer.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t get_wifi_location_fingerprint(char *location_name_buffer, size_t buffer_size);

/* * @brief Stops the Wi-Fi interface and deinitializes it.
 * 
 * This function stops the Wi-Fi interface and deinitializes it, releasing any resources used.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t stop_wifi();

#endif // WIFI_FINGERPRINT_H

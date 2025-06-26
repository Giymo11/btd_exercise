
#ifndef BT_HTTP_H
#define BT_HTTP_H

#include "freertos/semphr.h"
#include "esp_err.h"

extern SemaphoreHandle_t nvs_mutex;

/*
 * @brief Starts the HTTP server with the given SSID and password.
 *
 * This function initializes the HTTP server, sets up the Wi-Fi access point,
 * and registers URI handlers for various endpoints.
 *
 * @param ssid The SSID for the Wi-Fi access point.
 * @param password The password for the Wi-Fi access point.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t start_http_server(const char *ssid, const char *password);

/** @brief Stops the HTTP server and deinitializes the Wi-Fi access point.
 *
 * This function stops the HTTP server, destroys the Wi-Fi access point netif,
 * and deinitializes the Wi-Fi interface.
 *
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t stop_http_server();

#endif // BT_HTTP_H

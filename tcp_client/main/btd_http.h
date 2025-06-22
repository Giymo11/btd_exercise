
#ifndef BT_HTTP_H
#define BT_HTTP_H

#include "esp_err.h"

esp_err_t start_http_server(const char *ssid, const char *password);
esp_err_t stop_http_server();

#endif // BT_HTTP_H


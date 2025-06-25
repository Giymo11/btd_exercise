#include <string.h>
#include "freertos/FreeRTOS.h" // FreeRTOS API
#include "freertos/task.h"     // Task management
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "cJSON.h"

#include "btd_config.h"
#include "btd_http.h"
#include "btd_webui.h"
#include "btd_wifi.h"
#include "btd_stats.h"


static const char *TAG = "BTD_HTTP";

static esp_netif_t *netif = NULL; // Pointer to the network interface, if needed
static httpd_handle_t server = NULL;


// --- forward declarations of the handler functions
esp_err_t get_config_handler(httpd_req_t *req);
esp_err_t save_config_handler(httpd_req_t *req);
esp_err_t factoryreset_handler(httpd_req_t *req);
esp_err_t root_handler(httpd_req_t *req);
esp_err_t stats_handler(httpd_req_t *req);



esp_err_t start_wifi_ap(const char *ssid, const char *password)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    netif = esp_netif_create_default_wifi_ap();
    if (netif == NULL)
    {
        ESP_LOGE(TAG, "Failed to create default Wi-Fi AP netif");
        return ESP_FAIL;
    }

    wifi_config_t ap_config = {0};
    strncpy((char *)ap_config.ap.ssid, ssid, sizeof(ap_config.ap.ssid) - 1);
    strncpy((char *)ap_config.ap.password, password, sizeof(ap_config.ap.password) - 1);
    ap_config.ap.ssid_len = strlen(ssid);
    ap_config.ap.channel = 1;
    ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    ap_config.ap.max_connection = 4;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi AP configured. SSID: %s, PW: %s", ssid, password);
    return ESP_OK;
}

esp_err_t stop_wifi_ap()
{
    if (netif)
    {
        esp_netif_destroy_default_wifi(netif);
        netif = NULL;
        ESP_LOGI(TAG, "Wi-Fi AP netif destroyed");
    }
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());
    ESP_LOGI(TAG, "Wi-Fi AP stopped and deinitialized");
    return ESP_OK;
}

esp_err_t start_http_server(const char *ssid, const char *password)
{
    ESP_ERROR_CHECK(start_wifi_ap(ssid, password));

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_open_sockets = 4;
    config.max_uri_handlers = 8;

    // Start the HTTP server
    esp_err_t ret = httpd_start(&server, &config);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start HTTP server: %s", esp_err_to_name(ret));
        return ret;
    }

    httpd_uri_t uris[] = {
        {
            .uri       = "/config",
            .method    = HTTP_GET,
            .handler   = get_config_handler,
            .user_ctx  = NULL
        },
        {
            .uri       = "/config",
            .method    = HTTP_POST,
            .handler   = save_config_handler,
            .user_ctx  = NULL
        },
        {
            .uri       = "/factoryreset",
            .method    = HTTP_POST,
            .handler   = factoryreset_handler,
            .user_ctx  = NULL
        },
        {
            .uri       = "/",
            .method    = HTTP_GET,
            .handler   = root_handler,
            .user_ctx  = NULL
        },
        {
            .uri       = "/stats",
            .method    = HTTP_GET,
            .handler   = stats_handler,
            .user_ctx  = NULL
        }
    };
    // Register URI handlers
    for (int i = 0; i < sizeof(uris) / sizeof(uris[0]); i++)
    {
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uris[i]));
    }
    
    ESP_LOGI(TAG, "HTTP server started successfully");
    return ESP_OK;
}

esp_err_t stop_http_server()
{
    if (server)
    {
        httpd_stop(server);
        server = NULL;
        ESP_LOGI(TAG, "HTTP server stopped successfully");
    }
    ESP_ERROR_CHECK(stop_wifi_ap());
    ESP_LOGI(TAG, "Wi-Fi AP stopped successfully");
    return ESP_OK;
}

esp_err_t get_config_handler(httpd_req_t *req)
{
    btd_config_t config;
    cJSON *json = cJSON_CreateObject();
    char *response = NULL;
    esp_err_t resp_err = ESP_OK;

    // Create a JSON response with the configuration
    resp_err= btd_read_config(&config);
    if (resp_err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read configuration: %s", esp_err_to_name(resp_err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read configuration");
        goto cleanup;
    }

    if (!json)
    {
        ESP_LOGE(TAG, "Failed to create JSON object");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to create JSON object");
        resp_err = ESP_ERR_NO_MEM;
        goto cleanup;
    }

    cJSON_AddNumberToObject(json, "workTimeSeconds", config.workTimeSeconds);
    cJSON_AddNumberToObject(json, "breakTimeSeconds", config.breakTimeSeconds);
    cJSON_AddNumberToObject(json, "longBreakTimeSeconds", config.longBreakTimeSeconds);
    cJSON_AddNumberToObject(json, "longBreakSessionCount", config.longBreakSessionCount);
    cJSON_AddNumberToObject(json, "timeoutSeconds", config.timeoutSeconds);
    cJSON_AddBoolToObject(json, "breakGestureEnabled", config.breakGestureEnabled);

    response = cJSON_PrintUnformatted(json);
    if (!response)
    {
        ESP_LOGE(TAG, "Failed to print JSON object");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to print JSON object");
        resp_err = ESP_ERR_NO_MEM;
        goto cleanup;
    }

    resp_err = httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
    if (resp_err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(resp_err));
        goto cleanup;
    }

    ESP_LOGI(TAG, "Configuration sent successfully");

cleanup:
    if (response) free(response);
    if (json) cJSON_Delete(json);
    return resp_err;
}


esp_err_t factoryreset_handler(httpd_req_t *req)
{
    esp_err_t err = btd_delete_config(); // Reset configuration to factory defaults
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to reset configuration: %s", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to reset configuration");
        return err;
    }

    ESP_LOGI(TAG, "Configuration reset to factory defaults");
    httpd_resp_send(req, "Configuration reset to factory defaults", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}


esp_err_t root_handler(httpd_req_t *req)
{
    // const char *response = "<html><body><h1>Welcome to the BTD HTTP Server</h1>"
    //                       "<p>Use /config to get the current configuration.</p>"
    //                       "<p>Use /factoryreset to reset to factory defaults.</p></body></html>";

    const char *response = get_web_ui_html();
    return httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
}

esp_err_t save_config_handler(httpd_req_t *req)
{
    char content[256];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0)
    {
        ESP_LOGE(TAG, "Failed to receive request body");
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Failed to receive request body");
    }
    content[ret] = '\0'; // Null-terminate the received data

    cJSON *json = cJSON_Parse(content);
    if (!json)
    {
        ESP_LOGE(TAG, "Failed to parse JSON: %s", cJSON_GetErrorPtr());
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON format");
    }

    btd_config_t config;
    config.workTimeSeconds = cJSON_GetObjectItem(json, "workTimeSeconds")->valueint;
    config.breakTimeSeconds = cJSON_GetObjectItem(json, "breakTimeSeconds")->valueint;
    config.longBreakTimeSeconds = cJSON_GetObjectItem(json, "longBreakTimeSeconds")->valueint;
    config.longBreakSessionCount = cJSON_GetObjectItem(json, "longBreakSessionCount")->valueint;
    config.timeoutSeconds = cJSON_GetObjectItem(json, "timeoutSeconds")->valueint;
    config.breakGestureEnabled = cJSON_IsTrue(cJSON_GetObjectItem(json, "breakGestureEnabled"));

    esp_err_t err = btd_save_config(&config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to save configuration: %s", esp_err_to_name(err));
        cJSON_Delete(json);
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to save configuration");
    }

    cJSON_Delete(json);
    httpd_resp_send(req, "Configuration saved successfully", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// retuns all sessions as csv
esp_err_t stats_handler(httpd_req_t *req)
{
    session_stats_t stats[32]; // Array to hold session stats
    size_t count = sizeof(stats) / sizeof(stats[0]);
    esp_err_t err = get_all_work_sessions(stats, &count);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get work sessions: %s", esp_err_to_name(err));
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to get work sessions");
    }
    if (count == 0)
    {
        ESP_LOGI(TAG, "No work sessions found");
        return httpd_resp_send(req, "No work sessions found", HTTPD_RESP_USE_STRLEN);
    }
    // Create a CSV response
    char response[2048];
    size_t response_len = 0;
    response_len += snprintf(response, sizeof(response), "Session ID,Duration (s),Name,Mic Level\n");
    for (size_t i = 0; i < count; i++)
    {
        response_len += snprintf(response + response_len, sizeof(response) - response_len,
                                 "%lu,%lu,%s,%u\n",
                                 stats[i].session_id,
                                 stats[i].duration_seconds,
                                 stats[i].name,
                                 stats[i].mic_level);
        if (response_len >= sizeof(response))
        {
            ESP_LOGE(TAG, "Response buffer overflow");
            return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Response buffer overflow");
        }
    }
    // Send the CSV response
    esp_err_t resp_err = httpd_resp_send(req, response, response_len);
    if (resp_err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(resp_err));
        return resp_err;
    }
    ESP_LOGI(TAG, "Work sessions sent successfully");
    return ESP_OK;
}





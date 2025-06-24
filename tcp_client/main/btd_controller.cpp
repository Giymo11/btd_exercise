#include <string.h>
#include "freertos/FreeRTOS.h" // FreeRTOS API
#include "freertos/task.h"     // Task management
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_wifi.h"

#include "btd_vibration.c"
#include "nvs_flash.h"
#include "nvs.h"

#include "Arduino.h"
#include "M5StickCPlus.h"

#include "btd_battery.h"
#include "btd_display.h"
#include "btd_imu.h"
#include "btd_states.h"
#include "btd_button.h"
#include "btd_audio.h"
#include "btd_movement.h"

extern "C"
{
#include "btd_bandpass.h"
#include "btd_config.h"
#include "btd_http.h"
#include "btd_wifi.h"
}

#define INTERVAL 400
#define WAIT vTaskDelay(INTERVAL)
#define SAMPLING_FREQUENCY 100

static const char *TAG = "BTD_CONTROLLER";

static const int DELAY_BETWEEN_SAMPLES = 1000 / SAMPLING_FREQUENCY;

void init() // pls put all your inits here
{
    M5.begin();
    init_imu();
    setup_display();
    init_vibrator();
    ESP_ERROR_CHECK(nvs_flash_erase()); // IMPORTANT! Erase NVS at first startup, comment out to persist data
                                        // TODO Delete in final vers
    ESP_ERROR_CHECK(nvs_flash_init());
    // apparently its still needed??? even tho it errors? heck if I know
    // but yea, dont check if it errors - it just worksTM, sorry for the hack
    esp_event_loop_create_default();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_LOGI(TAG, "inits completed");
}

void test_config()
{
    btd_config_t config = {0};
    ESP_ERROR_CHECK(btd_read_config(&config));

    ESP_LOGI(TAG, "Configuration loaded: workTime=%d, breakTime=%d, longBreakTime=%d, longBreakSessionCount=%d, breakGestureEnabled=%d",
             config.workTimeSeconds, config.breakTimeSeconds, config.longBreakTimeSeconds,
             config.longBreakSessionCount, config.breakGestureEnabled);
}

void test_fingerprint()
{
    char location_name[32] = {0}; // [32] instead of the constant MAX_TEXT_CHARS
    ESP_ERROR_CHECK(get_wifi_location_fingerprint(location_name, sizeof(location_name)));
    ESP_LOGI(TAG, "Location fingerprint: %s", location_name);
}

void test_display()
{
    int test_time_sec = 4000;
    while (true)
    {
        clear_display();
        display_battery_percentage(get_battery_percentage());
        display_time(test_time_sec);
        test_time_sec--;
        display_break_bar();
        delay(1000);
        clear_display();
        display_break_msg();
        delay(1000);
        clear_display();
        display_battery_percentage(get_battery_percentage());
        display_qr_code();
        delay(1000);
    }
}

extern "C" void app_main(void)
{
    initArduino();

    static TickType_t last_wake_time = 0;

    printf("Arduino init\n");
    initArduino();
    printf("M5 init\n");

    M5.begin();
    printf("Display init start\n");

    setup_display();
    printf("Display init end\n");

    display_battery_percentage(get_battery_percentage());
    display_qr_code();

    ESP_LOGI(TAG, "Starting ti:ma");

    test_config();
    test_fingerprint();

    start_http_server("ti:ma", "12345678");
    ESP_LOGI(TAG, "HTTP server started");

    init_imu();
    float magnitude = getAccelMagnitude(); //@Lea - use it :)
    ESP_LOGI(TAG, "acceleration data: %f", magnitude);

    init_microphone();
    printf("Microphone init end\n");

    if (last_wake_time == 0)
    {
        last_wake_time = xTaskGetTickCount();
    }

    WAIT;
    printf("0\n");
    WAIT;

    init_movement_detection();

    // exec_vibration_pattern_a();
    while (true)
    {
        btn_detect_press();
        // delay exactly the right amount, and update last_wake_time
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(DELAY_BETWEEN_SAMPLES));

        bool is_above_threshold = is_volume_above_threshold(esp_timer_get_time() / 1000);

        // TODO: session startzeit und ende wird noch dafür gebraucht
        // float get_loud_percentage(session_start_ms, session_end_ms);

        bool walking = is_walking(getAccelMagnitude(), esp_timer_get_time() / 1000);

        bool break_gesture_detected = detect_break_gesture(getAccelMagnitude(), esp_timer_get_time() / 1000);

        bool auto_off = should_auto_off(getAccelMagnitude(), esp_timer_get_time() / 1000);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
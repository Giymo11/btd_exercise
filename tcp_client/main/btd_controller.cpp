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

extern "C"
{
#include "btd_bandpass.h"
#include "btd_config.h"
#include "btd_http.h"
#include "btd_wifi.h"
}

static const float WALKING_THRESHOLD = 0.12f;
static const float HPF_CUTOFF = 0.9f;
static const float LPF_CUTOFF = 3.6f;

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

extern "C" void app_main(void)
{
    initArduino();

    static TickType_t last_wake_time = 0;
    static bool was_walking = false;
    static bool was_stepping = false;
    static int64_t timestamp = 0; //@Lea - use it :)

    init();

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

    if (last_wake_time == 0)
    {
        last_wake_time = xTaskGetTickCount();
    }

    BandPassFilter lp, hp;
    init_lowpass(&lp, LPF_CUTOFF, (float)SAMPLING_FREQUENCY);  // 3 Hz cutoff frequency, 100 Hz sample rate
    init_highpass(&hp, HPF_CUTOFF, (float)SAMPLING_FREQUENCY); // 1 Hz cutoff frequency, 100 Hz sample rate

    WAIT;
    // old:
    // print_status(&dev, true, 0);
    printf("0\n");
    WAIT;

    int steps = 0;
    float mean_magnitude = 1.0449f;

    // exec_vibration_pattern_a();

    while (true)
    {

        btn_detect_press();
        // delay exactly the right amount, and update last_wake_time
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(DELAY_BETWEEN_SAMPLES));
        float rawValue = getAccelMagnitude() - mean_magnitude;
        float filteredValue = apply_filter(&hp, rawValue);
        filteredValue = apply_filter(&lp, filteredValue);
        // ESP_LOGI(TAG, "Raw: %.2f, Filtered: %.2f\n", rawValue, filteredValue);

        bool is_stepping = filteredValue > WALKING_THRESHOLD;

        if (is_stepping != was_stepping)
        {
            if (is_stepping)
            {
                ++steps;
                // printf("was_walking: %d, steps: %d\n", was_walking, steps);
            }
            was_stepping = is_stepping;
        }

        if (is_stepping)
        {
            was_walking = true;
            timestamp = esp_timer_get_time() / 1000;
        }
        else if (timestamp + 1500 < esp_timer_get_time() / 1000)
        {
            was_walking = false;
            // printf("Was_walking: %d\n", was_walking);
        }
    }

    // clear_display();
    // display_battery_percentage(get_battery_percentage());
    //  M5.Lcd.printf("Battery: %d%%\n", get_battery_percentage());
}


#include <string.h>

#include "freertos/FreeRTOS.h" // FreeRTOS API
#include "freertos/task.h"     // Task management
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "st7789.h"
#include "fontx.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_wifi.h"

#include "btd_bandpass.h"
#include "btd_config.h"
#include "btd_wifi.h"

#define INTERVAL 400
#define WAIT vTaskDelay(INTERVAL)

extern void test_screen(TFT_t *dev);
extern void init_imu(void);
extern float getAccelMagnitude();

#define SAMPLING_FREQUENCY 100
static const int DELAY_BETWEEN_SAMPLES = 1000 / SAMPLING_FREQUENCY;

static const float WALKING_THRESHOLD = 0.12f;
static const float HPF_CUTOFF = 0.9f;
static const float LPF_CUTOFF = 3.6f;

static const char *TAG = "BTD_MAIN";

static FontxFile typeface[2];
#define MAX_TEXT_CHARS 40
uint8_t text_to_display[MAX_TEXT_CHARS];

void print_status(TFT_t *dev, bool walking, int steps)
{
    uint16_t color;
    lcdFillScreen(dev, BLACK);
    lcdSetFontDirection(dev, 3);

    color = WHITE;
    uint16_t xpos = 100;
    uint16_t ypos = 200;

    if (walking)
    {
        snprintf((char *)text_to_display, MAX_TEXT_CHARS, "Im Walking Here!");
        lcdDrawString(dev, typeface, xpos, ypos, text_to_display, color);
    }
    snprintf((char *)text_to_display, MAX_TEXT_CHARS, "Steps: %d", steps);
    lcdDrawString(dev, typeface, xpos - 40, ypos, text_to_display, color);

    lcdDrawFinish(dev);
}

void init()
{
    // ESP_ERROR_CHECK(nvs_flash_erase()); // Erase NVS for testing purposes
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
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
    char location_name[LOCATION_NAME_MAX_LEN] = {0};
    ESP_ERROR_CHECK(get_wifi_location_fingerprint(location_name, sizeof(location_name)));
    ESP_LOGI(TAG, "Location fingerprint: %s", location_name);
}

void app_main(void)
{
    static TickType_t last_wake_time = 0;
    static bool was_walking = false;
    static bool was_stepping = false;
    static int64_t timestamp = 0;

    ESP_LOGI(TAG, "Starting ti:ma");
    init();

    test_config();
    test_fingerprint();


    TFT_t dev;
    test_screen(&dev);

    init_imu();
    float magnitude = getAccelMagnitude();
    ESP_LOGI(TAG, "acceleration data: %f", magnitude);

    if (last_wake_time == 0)
    {
        last_wake_time = xTaskGetTickCount();
    }

    BandPassFilter lp, hp;
    init_lowpass(&lp, LPF_CUTOFF, (float)SAMPLING_FREQUENCY);  // 3 Hz cutoff frequency, 100 Hz sample rate
    init_highpass(&hp, HPF_CUTOFF, (float)SAMPLING_FREQUENCY); // 1 Hz cutoff frequency, 100 Hz sample rate

    WAIT;
    InitFontx(typeface, "/fonts/ILGH24XB.FNT", ""); // Gothic
    print_status(&dev, true, 0);
    WAIT;

    lcdFillScreen(&dev, BLUE);
    lcdDrawFinish(&dev);

    int steps = 0;
    float mean_magnitude = 1.0449f;

    while (true)
    {
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
                print_status(&dev, was_walking, steps);
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
            print_status(&dev, was_walking, steps);
        }
    }
}
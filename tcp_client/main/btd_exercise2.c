

#include "esp_log.h" 
#include "st7789.h"

#include "btd_lowpass.h"

#define INTERVAL 400
#define WAIT vTaskDelay(INTERVAL)


extern void test_screen(TFT_t *dev);
extern void init_imu(void);
extern float getAccelMagnitude();

static const char *TAG = "BTD_MAIN";

void app_main(void)
{
    TFT_t dev;
    test_screen(&dev);

    init_imu();
    float magnitude = getAccelMagnitude();

    ESP_LOGI(TAG, "acceleration data: %f", magnitude);

    LowPassFilter lp;
    init_lowpass(&lp, 3.0f, 100.0f); // 3 Hz cutoff frequency, 1 kHz sample rate

    // Simulate reading raw data from a sensor
    for (int i = 0; i < 100; i++)
    {
        float rawValue = (rand() % 200) / 100.0f; // Replace with real sensor reading
        float filteredValue = filter_lowpass(&lp, rawValue);
        ESP_LOGI(TAG, "Raw: %.2f, Filtered: %.2f\n", rawValue, filteredValue);
    }

    WAIT;
    lcdFillScreen(&dev, BLUE);
    lcdDrawFinish(&dev);
}
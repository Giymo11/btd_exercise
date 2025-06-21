#include <string.h>
#include "freertos/FreeRTOS.h" // FreeRTOS API
#include "freertos/task.h"     // Task management
#include "esp_timer.h"
#include "esp_log.h"
#include "btd_vibration.c"

#include "Arduino.h"
#include "M5StickCPlus.h"

#include "btd_battery.h"
#include "btd_display.h"

#define INTERVAL 400
#define WAIT vTaskDelay(INTERVAL)

static const char *TAG = "BTD_MAIN";

extern "C" void app_main(void)
{
    printf("Arduino init\n");
    initArduino();
    printf("M5 init\n");

    M5.begin();
    printf("Display init start\n");

    setup_display();
    printf("Display init end\n");

    display_battery_percentage(get_battery_percentage());
    display_qr_code();
    clear_display();

    // init_vibrator();
    // exec_vibration_pattern_a();
    // clear_display();
    // display_battery_percentage(get_battery_percentage());
    //  M5.Lcd.printf("Battery: %d%%\n", get_battery_percentage());
}
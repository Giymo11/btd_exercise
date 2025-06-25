#include <string.h>
#include "freertos/FreeRTOS.h" // FreeRTOS API
#include "freertos/task.h"     // Task management
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_wifi.h"

#include "btd_vibrator.h"
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

static btd_state_t current_state = STATE_INIT;

static btd_config_t config = {0};
static int working_sec = 0;

static int break_sec = 0;
static bool break_gesture = true;
static int break_counter = 0;

static int longbreak_sec = 0;
static int longbreak_sess_count = 0;

// Wifi handler for checking if anyone connected to AP for automatic switch
static volatile bool someone_connected = false;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        someone_connected = true;
        ESP_LOGI(TAG, "A station connected to the AP");
    }
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        someone_connected = false;
        ESP_LOGI(TAG, "A station disconnected from the AP");
    }
}

// Sec. counter task START --------------------------
TaskHandle_t break_task_handle = NULL;
TaskHandle_t working_task_handle = NULL;

void countdown_task(void *pvParameter)
{
    int *seconds = (int *)pvParameter;
    while (1)
    {
        if (seconds && *seconds > 0)
        {
            (*seconds)--;
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // Wait 1 second
    }
}
// Sec. counter task END --------------------------

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
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &wifi_event_handler,
        NULL,
        NULL));
    init_microphone();
    init_movement_detection();

    ESP_LOGI(TAG, "inits completed");
}

// Tests START -------------------------------------------
void test_config()
{
    btd_config_t test_config = {0};
    ESP_ERROR_CHECK(btd_read_config(&test_config));

    ESP_LOGI(TAG, "Test Configuration loaded: workTime=%d, breakTime=%d, longBreakTime=%d, longBreakSessionCount=%d, breakGestureEnabled=%d",
             test_config.workTimeSeconds, test_config.breakTimeSeconds, test_config.longBreakTimeSeconds,
             test_config.longBreakSessionCount, test_config.breakGestureEnabled);
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
        display_wifi_code();
        delay(1000);
    }
}
// Tests END -------------------------------------------

// Awake state START -------------------------------------------
void start_awake()
{
    ESP_LOGI(TAG, "Start awake ");
    clear_display();
    start_http_server("ti:ma", "12345678");
    ESP_LOGI(TAG, "HTTP server started");
}

bool handle_awake()
{
    static int awake_step = 1;

    char btn = btn_detect_press();

    if (awake_step == 1)
    {
        display_wifi_code();
        display_battery_percentage(get_battery_percentage());
        if (someone_connected || btn == 'A')
        {
            clear_display();
            display_link_code();
            display_battery_percentage(get_battery_percentage());
            awake_step = 2;
        }
    }
    else if (awake_step == 2)
    {
        if (btn == 'A')
        {
            clear_display();
            awake_step = 1;
            return true; // -> Transition
        }
    }
    return false; // -> No transition
}

void stop_awake()
{
    stop_http_server();
    ESP_LOGI(TAG, "HTTP server stopped");
}

// Awake state END -------------------------------------------

void start_working()
{
    ESP_LOGI(TAG, "Start working");
    ESP_ERROR_CHECK(btd_read_config(&config));
    ESP_LOGI(TAG, "Prod. Configuration loaded: workTime=%d, breakTime=%d, longBreakTime=%d, longBreakSessionCount=%d, breakGestureEnabled=%d",
             config.workTimeSeconds, config.breakTimeSeconds, config.longBreakTimeSeconds,
             config.longBreakSessionCount, config.breakGestureEnabled);
    display_working_info_screen(get_battery_percentage());
    vTaskDelay(pdMS_TO_TICKS(1000));
    // vibration_pattern_a(); TODO fix vibrations
    working_sec = config.workTimeSeconds + 1;
    break_sec = config.breakTimeSeconds + 1;
    longbreak_sec = config.longBreakTimeSeconds + 1;
    longbreak_sess_count = config.longBreakSessionCount;
    break_gesture = config.breakGestureEnabled;
    xTaskCreate(countdown_task, "working_sec_countdown", 2048, &working_sec, 5, &working_task_handle);
}

bool handle_working()
{
    char btn = btn_detect_press();
    if (btn == 'B')
    {
        current_state = STATE_AWAKE;
    }

    float magnitude = getAccelMagnitude();
    int64_t timestamp = esp_timer_get_time() / 1000;

    bool is_above_threshold = is_volume_above_threshold(timestamp);
    bool walking = is_walking(magnitude, timestamp);
    bool break_gesture_detected = detect_break_gesture(magnitude, timestamp);
    bool auto_off = should_auto_off(magnitude, timestamp);

    if (is_above_threshold)
    {
        // TODO CSV protocol
    }

    if (walking)
    {
        // TODO CSV protocol walking
        return true;
    }

    if (break_gesture_detected)
    {
        // TODO CSV protocol break gesture
        return true;
    }

    if (auto_off)
    {
        // TODO correct auto off with saving data, protocolling etc.
        M5.Axp.PowerOff();
    }

    if (working_sec == 0)
    {
        return true;
    }

    return false;
}

void stop_working()
{
    ESP_LOGI(TAG, "Stop working");
    vTaskDelete(working_task_handle);
    working_task_handle = NULL;
}

// Working state END -------------------------------------------

// break state START -------------------------------------------

void start_break()
{
    display_break_info_screen(get_battery_percentage());
    vTaskDelay(pdMS_TO_TICKS(1000));
    xTaskCreate(countdown_task, "break_sec_countdown", 2048, &break_sec, 5, &break_task_handle);
}

bool handle_break()
{
    char btn = btn_detect_press();

    if (btn == 'B')
    {
        current_state = STATE_AWAKE;
    }

    if (break_sec == 0)
    {
        return true;
    }

    return false;
}

void stop_break()
{
    ESP_LOGI(TAG, "Stop break");
    vTaskDelete(break_task_handle);
    break_task_handle = NULL;
}

// break state END -------------------------------------------

extern "C" void app_main(void)
{
    initArduino();
    init();
    ESP_LOGI(TAG, "Starting ti:ma");

    static TickType_t last_wake_time = 0;

    test_config();
    test_fingerprint();

    current_state = STATE_AWAKE;

    btd_state_t last_state = (btd_state_t)-1;

    while (true)
    {
        // State transition handling
        if (current_state != last_state)
        {
            switch (last_state) // == STOP HANDLERS
            {
            case STATE_AWAKE:
                stop_awake();
                break;
            case STATE_WORKING:
                stop_working();
                break;
            case STATE_BREAK:
                stop_break();
                break;
            default:
                break;
            }

            switch (current_state) // == START HANDLERS
            {
            case STATE_AWAKE:
                start_awake();
                break;
            case STATE_WORKING:
                start_working();
                break;
            case STATE_BREAK:
                start_break();
                break;
            default:
                break;
            }
            last_state = current_state;
        }

        switch (current_state)
        {
        case STATE_AWAKE:
            if (handle_awake())
            {
                current_state = STATE_WORKING;
                last_wake_time = xTaskGetTickCount();
            }

            vTaskDelay(pdMS_TO_TICKS(200));
            break;
        case STATE_WORKING:
            static int last_displayed_working_sec = -1;
            if (handle_working())
            {
                current_state = STATE_BREAK;
                ESP_LOGI(TAG, "Stop Transitioning to break state.\n");
            }

            if (working_sec != last_displayed_working_sec)
            {
                display_working_time(working_sec, get_battery_percentage());
                last_displayed_working_sec = working_sec;
            }

            vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(DELAY_BETWEEN_SAMPLES));
            break;
        case STATE_BREAK:
            static int last_displayed_break_sec = -1;
            if (handle_break())
            {
                current_state = STATE_WORKING;
                ESP_LOGI(TAG, "Stop Transitioning to working state.\n");
            }

            if (break_sec != last_displayed_break_sec)
            {
                display_break_time(break_sec, get_battery_percentage());
                last_displayed_break_sec = break_sec;
            }

            vTaskDelay(pdMS_TO_TICKS(200));
            break;
        default:
            vTaskDelay(pdMS_TO_TICKS(200));
            break;
        }
    }
}
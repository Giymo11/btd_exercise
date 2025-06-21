#include "driver/ledc.h"
#include "freertos/FreeRTOS.h" // FreeRTOS API
#include "freertos/task.h"

#define BUZZER_GPIO 2
#define BUZZER_FREQ 2000

void init_buzzer(void)
{

    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = BUZZER_FREQ,
        .clk_cfg = LEDC_AUTO_CLK};
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .gpio_num = BUZZER_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 512, // 50% duty (range 0-1023)
        .hpoint = 0};
    ledc_channel_config(&ledc_channel);
}

static void turn_on_buzzer(void)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 512); // Set duty to 50%
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

static void turn_off_buzzer(void)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0); // Set duty to 0%
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

void exec_buzzer_pattern_a(void)
{
    turn_on_buzzer();
    vTaskDelay(pdMS_TO_TICKS(200));

    turn_off_buzzer();
    vTaskDelay(pdMS_TO_TICKS(200));

    turn_on_buzzer();
    vTaskDelay(pdMS_TO_TICKS(200));

    turn_off_buzzer();
    vTaskDelay(pdMS_TO_TICKS(200));

    turn_on_buzzer();
    vTaskDelay(pdMS_TO_TICKS(400));

    turn_off_buzzer();
}

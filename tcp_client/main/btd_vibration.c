#include "driver/gpio.h"
#include "freertos/FreeRTOS.h" // FreeRTOS API
#include "freertos/task.h"

static const gpio_num_t VIBRATOR_GPIO = GPIO_NUM_26;

void init_vibrator(void)
{
    gpio_reset_pin(VIBRATOR_GPIO);
    gpio_set_direction(VIBRATOR_GPIO, GPIO_MODE_OUTPUT);
}

void exec_vibration_pattern_a(void)
{
    gpio_set_level(VIBRATOR_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(200));

    gpio_set_level(VIBRATOR_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(200));

    gpio_set_level(VIBRATOR_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(200));

    gpio_set_level(VIBRATOR_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(200));

    gpio_set_level(VIBRATOR_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(400));

    gpio_set_level(VIBRATOR_GPIO, 0);
}

void exec_vibration_pattern_b(void)
{
    gpio_set_level(VIBRATOR_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(200));

    gpio_set_level(VIBRATOR_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(200));

    gpio_set_level(VIBRATOR_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(400));

    gpio_set_level(VIBRATOR_GPIO, 0);
}
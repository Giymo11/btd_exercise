#include <stdio.h>                             // standard input and output
#include "freertos/FreeRTOS.h"                 // for task and timing ops
#include "freertos/task.h"                     
#include "esp_random.h"                         // RNG


void app_main(void)
{
 int i = 0;
    while (1) {
    unsigned int randomNumber =  esp_random();
        printf("[%d] Hello world! %d\n", i, randomNumber);
        i++;
        vTaskDelay(5000 / portTICK_PERIOD_MS); // 5 second delay
    }
}
#include <M5StickCPlus.h>
#include "esp_log.h"
#include <math.h>

static const char *TAG = "IMU";

float getAccelMagnitude(void)
{
    float ax, ay, az;
    M5.IMU.getAccelData(&ax, &ay, &az);
    return sqrtf(ax * ax + ay * ay + az * az);
}

void init_imu(void)
{
    M5.Imu.Init();
    ESP_LOGI(TAG, "MPU6866 intialized successfully");
}

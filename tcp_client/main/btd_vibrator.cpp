#include <M5StickCPlus.h>
#include "driver/gpio.h"

#define CTRL_PIN 26

// https://docs.espressif.com/projects/arduino-esp32/en/latest/api/ledc.html
// https://github.com/m5stack/M5StickC-Plus/issues/29

enum Vibrate_mode_t
{
    stop = 0,
    weak,
    medium,
    strong,
    very_strong,
    strongest,
};
static uint8_t mode = stop;

void init_vibrator()
{
    bool status = ledcAttachChannel(CTRL_PIN, 500, 8, 2);
}

void vibration_pattern_a()
{
    printf("Start vibration");
    M5.update();
    ledcWriteChannel(2, 255);
    delay(200);
    M5.update();
    ledcWriteChannel(2, 0);
    delay(200);

    M5.update();
    ledcWriteChannel(2, 255);
    delay(200);

    M5.update();
    ledcWriteChannel(2, 0);
    delay(200);

    M5.update();
    ledcWriteChannel(2, 255);
    delay(400);

    M5.update();
    ledcWriteChannel(2, 0);
    M5.update();
    printf("Stop vibration");
}

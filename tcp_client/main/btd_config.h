#ifndef BTD_CONFIG_H
#define BTD_CONFIG_H

#include <stdint.h>

typedef struct {
    uint16_t workTimeSeconds;      // Work time in seconds
    uint16_t breakTimeSeconds;     // Break time in seconds
    uint16_t longBreakTimeSeconds; // Long break time in seconds
    uint16_t longBreakSessionCount;    // Interval for long breaks
    uint16_t timeoutSeconds;       // Timeout in seconds
    bool breakGestureEnabled;      // Break gesture on/off
} btd_config_t;

esp_err_t btd_read_config(btd_config_t *config);
esp_err_t btd_save_config(const btd_config_t *config);

#endif // BTD_CONFIG_H
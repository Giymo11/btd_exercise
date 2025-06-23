#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "btd_bandpass.h"

#ifdef __cplusplus
extern "C" {
#endif


bool detect_movement(float magnitude);
void init_movement_detection();
/*
    
*/
bool is_walking(float magnitude, int64_t current_time_ms);
bool detect_break_gesture(float magnitude, int64_t timestamp);
bool should_auto_off(float magnitude, int64_t timestamp);

#ifdef __cplusplus
}
#endif
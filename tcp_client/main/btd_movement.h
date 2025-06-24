#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "btd_bandpass.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
    In: magnitude
    Out: is true if movement is detected (detect over Movement threshold)
*/
bool detect_movement(float magnitude);

/*
    initializes the movement bandpass filter
*/
void init_movement_detection();

/*
    In: current timestamp and magnitude
    Out: is true if 5 steps occur in more than 3 secs and less than 5 secs
*/
bool is_walking(float magnitude, int64_t current_time_ms);

/*
    In: current timestamp and magnitude
    Out: is true if more than 20 times over break gesture threshold in 1,5 secs
*/
bool detect_break_gesture(float magnitude, int64_t timestamp);

/*
    In: current timestamp and magnitude
    Out: is true if no movement was detected in the last 5 mins
*/
bool should_auto_off(float magnitude, int64_t timestamp);

#ifdef __cplusplus
}
#endif
#include "btd_movement.h"
#include "btd_bandpass.h"
#include <cmath>
#include <chrono>

extern "C" {
    #include "esp_timer.h" 
    #include "btd_bandpass.h"
}

#define SAMPLING_FREQUENCY 100

static uint64_t last_movement_time_ms = 0;
static const float MOVEMENT_THRESHOLD = 0.15f; 
static const uint64_t TIMEOUT_MS = 5 * 60 * 1000; 

static const float WALKING_THRESHOLD = 0.08f;
static const float HPF_CUTOFF = 1.5f;
static const float LPF_CUTOFF = 2.0f;
static const float mean_magnitude = 1.0449f;

static bool was_stepping = false;
static const int MIN_STEP_INTERVAL_MS = 500; // at least 500ms between 2 steps
static uint64_t last_step_time_ms = 0;
static uint64_t fist_step_time_ms = 0;
int steps = 0;

static BandPassFilter lp, hp;
static BandPassFilter break_filter_lp, break_filter_hp;

static int peak_count = 0;
static uint64_t first_peak_time = 0;
static const float BREAK_GESTURE_LOWPASS = 6.5f;
static const float BREAK_GESTURE_HIGHPASS = 2.0f;
static const float BREAK_PEAK_THRESHOLD = 0.15f; 
static const int BREAK_GESTURE_WINDOW_MS = 1500; // 1500ms 

bool detect_movement(float magnitude) {
    return fabs(magnitude - 1.0f) > MOVEMENT_THRESHOLD; 
}

void init_movement_detection() {
     //for Walking:
    init_lowpass(&lp, LPF_CUTOFF, (float)SAMPLING_FREQUENCY);
    init_highpass(&hp, HPF_CUTOFF, (float)SAMPLING_FREQUENCY);

    //for Break Gesture:
    init_lowpass(&break_filter_lp, BREAK_GESTURE_LOWPASS, (float)SAMPLING_FREQUENCY);
    init_highpass(&break_filter_hp, BREAK_GESTURE_HIGHPASS, (float)SAMPLING_FREQUENCY);
}

bool should_auto_off(float magnitude, int64_t current_time_ms){
    if (detect_movement(magnitude)) {
        last_movement_time_ms = current_time_ms;
        return false;  
    }
    if (last_movement_time_ms == 0) {
        last_movement_time_ms = current_time_ms;
        return false;
    }
    if ((current_time_ms - last_movement_time_ms) >= TIMEOUT_MS) {
        return true; 
    }
    return false;
}

bool is_walking(float magnitude, int64_t current_time_ms) {
    float rawValue = magnitude - mean_magnitude;
    float filteredValue = apply_filter(&hp, rawValue);
    filteredValue = apply_filter(&lp, filteredValue);

    bool is_stepping = fabs(filteredValue) > WALKING_THRESHOLD;

    if (is_stepping && !was_stepping) {
        if (current_time_ms - last_step_time_ms > MIN_STEP_INTERVAL_MS) {
            last_step_time_ms = current_time_ms;
            if (steps == 0) {
                fist_step_time_ms = current_time_ms;
            }
            steps++;
        }
    }
    was_stepping = is_stepping;

    if (steps >= 5 ) {
        if((current_time_ms - fist_step_time_ms) <= 5000 && (current_time_ms - fist_step_time_ms) >= 3000){
            printf("walking detected!\n");
            steps = 0;
            fist_step_time_ms = 0;
            return true;
        } else {
            //reset steps if they did not happen in more than 3 secs and less than 5 secs
            steps = 0;
            fist_step_time_ms = 0;
        }
    }
    return false;
}

bool detect_break_gesture(float magnitude, int64_t current_time_ms) {
    float rawValue = magnitude - mean_magnitude;
    float filteredValue = apply_filter(&break_filter_hp, rawValue);
    filteredValue = apply_filter(&break_filter_lp, filteredValue);

    if (fabs(filteredValue) > BREAK_PEAK_THRESHOLD) {
        if (peak_count == 0) {
            first_peak_time = current_time_ms;
        }
        if ((current_time_ms - first_peak_time) <= BREAK_GESTURE_WINDOW_MS) {
            peak_count++;
        } else {
            if (peak_count >= 20) {
                peak_count = 0;
                first_peak_time = 0;
                printf("break-gesture detected! \n");
                return true;
            }
            peak_count = 1;
            first_peak_time = current_time_ms;
        }
    }
    return false;
}



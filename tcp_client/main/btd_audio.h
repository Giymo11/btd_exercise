#pragma once

/*
    initializes the microphone
*/
void init_microphone();

/*
    In: current timestamp
    Out: is true if the detected volume is above the defined threshold (currently 2500)
*/
bool is_volume_above_threshold(int64_t current_time_ms);

/*
    In: session start timestamp and session end timestamp
    Out: float value of percentage of duration of the session which is louder than threshold 
*/
float get_loud_percentage(int64_t session_start_ms, int64_t session_end_ms);

/*
    Out: int value of volume loudness data 
*/
int read_microphone_sample();  

/*
    Out: total duration of time which is louder than threshold (in ms)
*/
int64_t get_total_loud_duration_ms();


#pragma once

#ifndef BTD_BANDPASS_H
#define BTD_BANDPASS_H

#ifdef __cplusplus
extern "C" {
#endif

// Second-order Low-Pass Filter Struct
typedef struct {
    float a[2]; // Feedback coefficients
    float b[3]; // Feedforward coefficients
    float x[3]; // Input (raw) values
    float y[3]; // Output (filtered) values
    float omega0; // Angular frequency
    float dt; // Time step
} BandPassFilter;

void init_lowpass(BandPassFilter *filter, float cutoff_frequency, float sample_frequency);
void init_highpass(BandPassFilter *filter, float cutoff_frequency, float sample_frequency);
float apply_filter(BandPassFilter *filter, float raw_value);

#ifdef __cplusplus
}
#endif

#endif // BTD_BANDPASS_H


#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "btd_bandpass.h"


// this is using a lot of LLM-generated code based on https://github.com/curiores/ArduinoTutorials/blob/main/BasicFilters/ArduinoImplementations/LowPass/LowPass2.0/LowPass2.0.ino

// Initialize the filter
void init_lowpass(BandPassFilter *filter, float cutoff_frequency, float sample_frequency) {
    // Calculate angular frequency
    filter->omega0 = 2 * M_PI * cutoff_frequency;
    filter->dt = 1.0 / sample_frequency;

    // Initialize arrays
    for (int i = 0; i < 3; i++) {
        filter->x[i] = 0.0;
        filter->y[i] = 0.0;
    }

    // Calculate coefficients
    float alpha = filter->omega0 * filter->dt;
    float alphaSq = alpha * alpha;
    float beta[3] = {1, sqrt(2), 1};
    float D = alphaSq * beta[0] + 2 * alpha * beta[1] + 4 * beta[2];
    
    // Feedforward coefficients
    filter->b[0] = alphaSq / D;
    filter->b[1] = 2 * filter->b[0];
    filter->b[2] = filter->b[0];

    // Feedback coefficients
    filter->a[0] = -(2 * alphaSq * beta[0] - 8 * beta[2]) / D;
    filter->a[1] = -(beta[0] * alphaSq - 2 * beta[1] * alpha + 4 * beta[2]) / D;
}

// Apply filter to the new input value
float apply_filter(BandPassFilter *filter, float raw_value) {
    // Shift historical values
    for (int i = 2; i > 0; i--) {
        filter->x[i] = filter->x[i - 1];
        filter->y[i] = filter->y[i - 1];
    }

    // New input value
    filter->x[0] = raw_value;

    // Compute the filtered output value
    filter->y[0] = filter->b[0] * filter->x[0] + filter->b[1] * filter->x[1] + filter->b[2] * filter->x[2]
                   + filter->a[0] * filter->y[1] + filter->a[1] * filter->y[2];


    // Return the filtered value
    return filter->y[0];
}


// Function to initialize the filter
void init_highpass(BandPassFilter *filter, float cutoff_frequency, float sample_frequency) {
    // Calculate angular frequency
    filter->omega0 = 2 * M_PI * cutoff_frequency;
    filter->dt = 1.0 / sample_frequency;

    // Initialize arrays
    for (int i = 0; i < 3; i++) {
        filter->x[i] = 0.0;
        filter->y[i] = 0.0;
    }

    // Calculate coefficients
    float alpha = filter->omega0 * filter->dt;

    float alphaFactor = 1.0 / (1.0 + alpha / 2.0);
    filter->a[0] = -(alpha / 2.0 - 1) * alphaFactor;
    filter->b[0] = alphaFactor;
    filter->b[1] = -alphaFactor;
}

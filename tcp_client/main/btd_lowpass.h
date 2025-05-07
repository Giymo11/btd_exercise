
#pragma once

// Define the filter structure
typedef struct
{
    float a[2];   // Coefficients for feedback
    float b[3];   // Coefficients for feedforward
    float dt;     // Time interval
    float omega0; // Cutoff frequency
    float tn1;    // Last time
    float x[3];   // Raw input values (buffer)
    float y[3];   // Filtered output values (buffer)
} LowPassFilter;

void init_lowpass(LowPassFilter *lp, float f0, float fs);
float filter_lowpass(LowPassFilter *lp, float xn);



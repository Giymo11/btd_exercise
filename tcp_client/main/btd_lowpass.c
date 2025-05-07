#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "btd_lowpass.h"


// Set coefficients based on cutoff frequency and sampling frequency
void LowPassFilter_SetCoef(LowPassFilter *lp)
{
    float alpha = lp->omega0 * lp->dt;

    // First-order filter
    lp->a[0] = -(alpha - 2.0) / (alpha + 2.0);
    lp->b[0] = alpha / (alpha + 2.0);
    lp->b[1] = lp->b[0]; // First-order requires two b coefficients
}

// Initialize the filter
void init_lowpass(LowPassFilter *lp, float f0, float fs)
{
    lp->omega0 = 2.0 * M_PI * f0; // 2 * pi * f0
    lp->dt = 1.0 / fs;            // Sample time interval
    lp->tn1 = -lp->dt;               // Initialize tn1
    memset(lp->x, 0, sizeof(lp->x)); // Initialize buffers to 0
    memset(lp->y, 0, sizeof(lp->y)); // Initialize buffers to 0
    LowPassFilter_SetCoef(lp);
}

// Apply filter to the new input value
float filter_lowpass(LowPassFilter *lp, float xn)
{
    // Shift historical values
    for (int k = 2; k > 0; k--)
    {
        lp->y[k] = lp->y[k - 1];
        lp->x[k] = lp->x[k - 1];
    }

    lp->x[0] = xn; // Store new input value

    // Compute filtered output
    lp->y[0] = lp->b[0] * lp->x[0];
    for (int k = 0; k < 2; k++)
    {
        lp->y[0] += lp->a[k] * lp->y[k + 1];
    }

    return lp->y[0]; // Return filtered value
}


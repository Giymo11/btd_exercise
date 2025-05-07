| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C5 | ESP32-C6 | ESP32-C61 | ESP32-H2 | ESP32-P4 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | --------- | -------- | -------- | -------- | -------- |



## what i did

- set com port
- set device target
- add baud rate to settings.json



# exercise 2

all relevant code is in files starting with "btd_"
the code for streaming to the server is still in tcp_client_v4.c

## reflection

### What values did you obtain for cutoff frequencies?

in papers for gait detection i found values for these second order low pass filters at around 3 to 6 Hz
looking at only the analysis of my recorded data, the peak is clearly at abuot 2 Hz. playing around with different frequencies, i could not really detect any strong difference in the clarity of the signal between values at 4 Hz (Nyquist), at 2.5 Hz (slightly above the target) or directly at 2, as the attenuation in this transition band is not so harsh with this 2nd order filter. Also, for the high pass i tried 0.2, 1, 1.5, 2 Hz - i did not notice the signal getting a lot clearer or worse either way. 

So in the end I chose a middle way: 1 Hz for high-pass and 3 for low-pass.


### How could these cutoff frequencies be adjusted to detect different activities, such as running?




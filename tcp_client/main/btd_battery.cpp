#include "Arduino.h"
#include "M5StickCPlus.h"

int get_battery_percentage(void)
{
    float voltage = M5.Axp.GetBatVoltage();
    int percentage = (voltage - 3.0) / (4.2 - 3.0) * 100; // formula from ChatGPT
    if (percentage < 0)
        percentage = 0;
    if (percentage > 100)
        percentage = 100;
    return percentage;
}
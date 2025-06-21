#include <M5StickCPlus.h>

// detect button press
char btn_detect_press(void)
{
    M5.update(); // needed for reading out the button vals
    if (M5.BtnA.wasReleased())
    {
        // printf("PRESSED: A\n");
        return 'A';
    }
    else if (M5.BtnB.wasReleased())
    {
        // printf("PRESSED: B\n");
        return 'B';
    }
    return 'X';
}

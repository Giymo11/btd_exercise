#include "Arduino.h"
#include "M5StickCPlus.h"
#include "btd_qr.h"
// display size 135 x 240

void setup_display(void)
{
    M5.Lcd.begin();
    M5.Lcd.setRotation(3);

    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0, 1);
}

void clear_display(void)
{
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0, 1);
}

void display_battery_percentage(int percentage)
{
    M5.Lcd.setTextColor(BLUE);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(10, 10, 2);
    M5.Lcd.printf("%d%%\n", percentage);
    M5.Lcd.setTextColor(WHITE);
}

void display_wifi_code(void)
{
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 50, 2);
    M5.Lcd.print("Conn.");
    M5.Lcd.pushImage(100, 0, 135, 135, wifi_code);
}

void display_link_code(void)
{
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 50, 2);
    M5.Lcd.print("Open");
    M5.Lcd.pushImage(100, 0, 135, 135, link_code);
}

void display_time(int sec)
{
    M5.Lcd.setTextSize(4);
    int minutes = sec / 60;
    int seconds = sec % 60;
    M5.Lcd.setCursor(55, 40, 2);

    M5.Lcd.printf("%02d:%02d\n", minutes, seconds);
}

void display_break_msg(void)
{
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(55, 50, 2);
    M5.Lcd.setTextColor(GREENYELLOW);
    M5.Lcd.print("Break time!");
    M5.Lcd.setTextColor(WHITE);
}

void display_break_bar(void)
{
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(190, 10, 2);
    M5.Lcd.setTextColor(GREENYELLOW);
    M5.Lcd.print("BREAK");
    M5.Lcd.setTextColor(WHITE);
}

void display_break_info_screen(int battery)
{
    clear_display();
    display_break_msg();
    display_battery_percentage(battery);
}

void display_working_msg(void)
{
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(55, 50, 2);
    M5.Lcd.setTextColor(PINK);
    M5.Lcd.print("Focus time!");
    M5.Lcd.setTextColor(WHITE);
}

void display_working_bar(void)
{
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(190, 10, 2);
    M5.Lcd.setTextColor(PINK);
    M5.Lcd.print("FOCUS");
    M5.Lcd.setTextColor(WHITE);
}

void display_break_time(int break_sec, int battery)
{
    clear_display();
    display_time(break_sec);
    display_break_bar();
    display_battery_percentage(battery);
}

void display_working_info_screen(int battery)
{
    clear_display();
    display_working_msg();
    display_battery_percentage(battery);
}

void display_working_time(int working_sec, int battery)
{
    clear_display();
    display_time(working_sec);
    display_working_bar();
    display_battery_percentage(battery);
}

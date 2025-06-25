#pragma once
void setup_display(void);
void clear_display(void);
void display_battery_percentage(int percentage);
void display_wifi_code(void);
void display_link_code(void);
void display_time(int sec);
void display_break_msg(void);
void display_break_bar(void);
void display_break_info_screen(int battery);
void display_break_time(int break_sec, int battery);
void display_working_msg(void);
void display_working_bar(void);
void display_working_info_screen(int battery);
void display_working_time(int working_sec, int battery);
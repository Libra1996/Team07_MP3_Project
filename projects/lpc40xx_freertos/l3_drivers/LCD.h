#include "FreeRTOS.h"
#include <stdbool.h>

void clear();
void home();
void noDisplay();
void display();
void noBlink();
void blink();
void noCursor();
void cursor();
void scrollDisplayLeft();
void scrollDisplayRight();
void printLeft();
void printRight();
void leftToRight();
void rightToLeft();
void shiftIncrement();
void shiftDecrement();
void noBacklight();
void backlight();
void autoscroll();
void noAutoscroll();
void createChar(uint8_t, uint8_t[]);
void setCursor(uint8_t, uint8_t);
void command(uint8_t);
void LCD_init(uint8_t, uint8_t, uint8_t);

void load_custom_character(uint8_t char_num, uint8_t *rows); // alias for createChar()

void send(uint8_t, uint8_t);
void write4bits(uint8_t);
void expanderWrite(uint8_t);
void pulseEnable(uint8_t);

// LCD design
void load_all_custom();
void LCD_home_menu();
void LCD_song_list();
void LCD_play(char current[]);
void helper(char s[], uint8_t len);
void LCD_play_pause(bool flag);
void LCD_bass_treble();
void bass_treble_cursor(bool flag);
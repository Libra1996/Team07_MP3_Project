#include "FreeRTOS.h"
#include "delay.h"
#include "i2c.h"
#include "song_list.h"
#include <string.h>

#define I2C_2 2

// commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

// flags for backlight control
#define LCD_BACKLIGHT 0x08
#define LCD_NOBACKLIGHT 0x00

#define En 0x04 // Enable bit
#define Rw 0x02 // Read/Write bit
#define Rs 0x01 // Register select bit

uint8_t _Addr;
uint8_t _displayfunction = LCD_4BITMODE | LCD_2LINE | LCD_5x8DOTS;
uint8_t _displaycontrol;
uint8_t _displaymode;
uint8_t _numlines;
uint8_t _cols;
uint8_t _rows;
uint8_t _backlightval;

void expanderWrite(uint8_t data) { i2c__write_LCD(I2C_2, _Addr, (int)(data) | _backlightval); }

void pulseEnable(uint8_t data) {
  expanderWrite(data | En); // En high
  delay__ms(1);             // enable pulse must be >450ns

  expanderWrite(data & ~En); // En low
  delay__ms(50);             // commands need > 37us to settle
}

void write4bits(uint8_t value) {
  expanderWrite(value);
  pulseEnable(value);
}

/************ low level data pushing commands **********/

// write either command or data
void send(uint8_t value, uint8_t mode) {
  uint8_t highnib = value & 0xf0;
  uint8_t lownib = (value << 4) & 0xf0;
  write4bits((highnib) | mode);
  write4bits((lownib) | mode);
}

/*********** mid level commands, for sending data/cmds */

void command(uint8_t value) { send(value, 0); }

/********** high level commands, for the user! */
void clear() {
  command(LCD_CLEARDISPLAY); // clear display, set cursor position to zero
  delay__ms(2);              // this command takes a long time!
}

void home() {
  command(LCD_RETURNHOME); // set cursor position to zero
  delay__ms(2);            // this command takes a long time!
}

void setCursor(uint8_t col, uint8_t row) {
  int row_offsets[] = {0x00, 0x40, 0x14, 0x54};
  if (row > _numlines) {
    row = _numlines - 1; // we count rows starting w/0
  }
  command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

// Turn the display on/off (quickly)
void noDisplay() {
  _displaycontrol &= ~LCD_DISPLAYON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void display() {
  _displaycontrol |= LCD_DISPLAYON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn the (optional) backlight off/on
void noBacklight(void) {
  _backlightval = LCD_NOBACKLIGHT;
  expanderWrite(0);
}

void backlight(void) {
  _backlightval = LCD_BACKLIGHT;
  expanderWrite(0);
}

// Turns the underline cursor on/off
void noCursor() {
  _displaycontrol &= ~LCD_CURSORON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void cursor() {
  _displaycontrol |= LCD_CURSORON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void noBlink() {
  _displaycontrol &= ~LCD_BLINKON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void blink() {
  _displaycontrol |= LCD_BLINKON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// These commands scroll the display without changing the RAM
void scrollDisplayLeft(void) { command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT); }
void scrollDisplayRight(void) { command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT); }

// This is for text that flows Left to Right
void leftToRight(void) {
  _displaymode |= LCD_ENTRYLEFT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// This is for text that flows Right to Left
void rightToLeft(void) {
  _displaymode &= ~LCD_ENTRYLEFT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'right justify' text from the cursor
void autoscroll(void) {
  _displaymode |= LCD_ENTRYSHIFTINCREMENT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'left justify' text from the cursor
void noAutoscroll(void) {
  _displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void createChar(uint8_t location, uint8_t charmap[]) {
  location &= 0x7; // we only have 8 locations 0-7
  command(LCD_SETCGRAMADDR | (location << 3));
  for (int i = 0; i < 8; i++) {
    send(charmap[i], Rs);
  }
}

void LCD_init(uint8_t slave_addr, uint8_t cols, uint8_t rows) {
  _numlines = rows;
  _Addr = slave_addr;

  // SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
  // according to datasheet, we need at least 40ms after power rises above 2.7V
  // before sending commands. Arduino can turn on way befer 4.5V so we'll wait 50
  delay__ms(50);

  // Now we pull both RS and R/W low to begin commands
  expanderWrite(_backlightval); // reset expanderand turn backlight off (Bit 8 =1)
  delay__ms(1000);

  // put the LCD into 4 bit mode
  // this is according to the hitachi HD44780 datasheet
  // figure 24, pg 46

  // we start in 8bit mode, try to set 4 bit mode
  write4bits(0x03 << 4);
  delay__ms(5); // wait min 4.1ms

  // second try
  write4bits(0x03 << 4);
  delay__ms(5); // wait min 4.1ms

  // third go!
  write4bits(0x03 << 4);
  delay__ms(1);

  // finally, set to 4-bit interface
  write4bits(0x02 << 4);

  // set # lines, font size, etc.
  command(LCD_FUNCTIONSET | _displayfunction);

  // turn the display on with no cursor or blinking default
  _displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
  display();

  // clear it off
  clear();

  // Initialize to default text direction (for roman languages)
  _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;

  // set the entry mode
  command(LCD_ENTRYMODESET | _displaymode);

  backlight();

  home();
}

// Alias functions
void load_custom_character(uint8_t char_num, uint8_t *rows) { createChar(char_num, rows); }

void helper(char s[], uint8_t len) {
  if (len == 0) {
    for (uint8_t i = 0; i < strlen(s); i++) {
      send(s[i], Rs);
    }
  } else {
    for (uint8_t i = 0; i < len; i++) {
      send(s[i], Rs);
    }
  }
}

// Custom Character
uint8_t play_char[] = {0x00, 0x08, 0x0C, 0x0E, 0x0F, 0x0E, 0x0C, 0x08};
uint8_t pause_char[] = {0x00, 0x00, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x00};
uint8_t prev_char[] = {0x00, 0x11, 0x13, 0x17, 0x1F, 0x17, 0x13, 0x11};
uint8_t next_char[] = {0x00, 0x11, 0x19, 0x1D, 0x1F, 0x1D, 0x19, 0x11};
uint8_t vol_1[] = {0x01, 0x03, 0x07, 0x1F, 0x1F, 0x07, 0x03, 0x01};
uint8_t vol_2[] = {0x00, 0x01, 0x05, 0x15, 0x15, 0x05, 0x01, 0x00};
uint8_t setting_char[] = {0x00, 0x15, 0x0A, 0x11, 0x11, 0x0A, 0x15, 0x00};
uint8_t list_char[] = {0x00, 0x1F, 0x11, 0x1F, 0x11, 0x1F, 0x11, 0x1F};

void load_all_custom() {
  load_custom_character(0, play_char);
  delay__ms(1);
  load_custom_character(1, pause_char);
  delay__ms(1);
  load_custom_character(2, setting_char);
  delay__ms(1);
  load_custom_character(3, list_char);
  delay__ms(1);
  load_custom_character(4, prev_char);
  delay__ms(1);
  load_custom_character(5, next_char);
  delay__ms(1);
  load_custom_character(6, vol_1);
  delay__ms(1);
  load_custom_character(7, vol_2);
  home();
}

// LCD design
void LCD_home_menu() {
  clear();
  setCursor(5, 0);
  helper("+Anh--Gia+", 0);
  setCursor(6, 1);
  helper("CMPE 146", 0);
  setCursor(5, 3);
  helper("Spring2022", 0);
}

void print_song_name(size_t song_number) {
  char *s = song_list__get_name_for_item(song_number);
  int song_name_count = 0;
  for (uint8_t i = 0; i < strlen(s); i++) {
    if (s[i] == '.') {
      break;
    }
    song_name_count++;
  }

  helper(s, song_name_count);
}

void LCD_song_list(uint8_t range_1, uint8_t range_2) {
  clear();
  setCursor(5, 0);
  helper("Song  List", 0);
  uint8_t count = range_1;
  for (uint8_t i = 0; i < 3; i++) {
    setCursor(1, i + 1);
    if (count <= range_2) {
      print_song_name(count);
      count++;
    }
  }
}

void LCD_play(char current[]) {
  clear();
  setCursor(3, 0);
  helper("Anh ", 0);
  send(0x26, Rs);
  helper(" Gia  MP3", 0);
  setCursor(0, 1);
  helper("Playing...", 0);
  int count = 0;
  for (uint8_t i = 0; i < strlen(current); i++) {
    if (current[i] == '.') {
      break;
    }
    count++;
  }
  setCursor(1, 2);
  helper(current, count);
  setCursor(2, 3);
  send(6, Rs);
  send(7, Rs);
  setCursor(8, 3);
  send(4, Rs);
  setCursor(12, 3);
  send(5, Rs);
  setCursor(16, 3);
  send(2, Rs);
  send(' ', Rs);
  send(3, Rs);
}

void LCD_play_pause(bool flag) {
  setCursor(10, 3);
  send(' ', Rs);
  setCursor(10, 3);
  if (flag == false) {
    send(0, Rs);
  } else {
    send(1, Rs);
  }
}

void LCD_bass_treble() {
  clear();
  setCursor(6, 0);
  helper("Setting", 0);
  setCursor(2, 2);
  helper("Bass", 0);
  setCursor(11, 2);
  helper("Treble", 0);
}

void bass_treble_cursor(bool flag) {
  if (flag) {
    setCursor(1, 2);
  } else {
    setCursor(10, 2);
  }

  send(0, Rs);
}
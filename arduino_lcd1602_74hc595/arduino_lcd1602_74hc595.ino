// pinout is
// 6: output, latch
// 11: output, spi mosi (master out, slave in)
// 13: output, spi clock
#include <SPI.h>
constexpr int LCD_LATCH_PIN = 6;

// custom characters
const char CHR_OPEN_LOCK = 0x00;
const char CHR_CLOSED_LOCK = 0x01;
const char CHR_THERMOMETER = 0x02;
const char CHR_HUMIDITY = 0x03;
const char CHR_DEGREE = 0b11011111;
const char CHR_MAIL = 0x04;
const char CHR_LEFT_SELECT = 0x05;
const char CHR_TICK = 0x06;
const char CHR_UP = 0x07;
const char CHR_RIGHT_ARROW = 0b01111110;
const char CHR_LEFT_ARROW = 0b01111111;
// Creator website: https://maxpromer.github.io/LCD-Character-Creator/
constexpr int LCD_CHARACTER_HEIGHT = 8;
static const uint8_t DATA_OPEN_LOCK[LCD_CHARACTER_HEIGHT] = {0x0E,0x01,0x01,0x01,0x1F,0x1B,0x1B,0x0E};
static const uint8_t DATA_CLOSED_LOCK[LCD_CHARACTER_HEIGHT] = {0x0E,0x11,0x11,0x11,0x1F,0x1B,0x1B,0x0E};
static const uint8_t DATA_THERMOMETER_SYMBOL[LCD_CHARACTER_HEIGHT] = {0x04,0x0A,0x0A,0x0A,0x0E,0x1F,0x1F,0x0E};
static const uint8_t DATA_HUMIDITY_SYMBOL[LCD_CHARACTER_HEIGHT] = {0x04,0x04,0x0A,0x0A,0x11,0x11,0x11,0x0E};
static const uint8_t DATA_DEGREE_SYMBOL[LCD_CHARACTER_HEIGHT] = {0x1C,0x14,0x1C,0x00,0x00,0x00,0x00,0x00};
static const uint8_t DATA_MAIL_SYMBOL[LCD_CHARACTER_HEIGHT] = {0x00,0x0E,0x1F,0x1F,0x15,0x11,0x1F,0x00};
static const uint8_t DATA_LEFT_SELECT_SYMBOL[LCD_CHARACTER_HEIGHT] = {0x01,0x01,0x05,0x0D,0x1F,0x0C,0x04,0x00};
static const uint8_t DATA_TICK_SYMBOL[LCD_CHARACTER_HEIGHT] = {0x01,0x01,0x03,0x03,0x16,0x1E,0x1C,0x08};
static const uint8_t DATA_UP_SYMBOL[LCD_CHARACTER_HEIGHT] = {0x04,0x0E,0x1F,0x04,0x04,0x04,0x04,0x04};

// shift register (74hc595) and lcd (lcd1602)
// shift register output pin assignment to lcd1602 input pins
// assigned to Q0 to Q7 of shift register
// D4 = least significant bit (Q4)
// D7 = most significant bit  (Q7)
constexpr int SR_SELECT_PIN = 1;
constexpr int SR_ENABLE_PIN = 3;
constexpr int SR_TOTAL_DATA_PINS = 4;
static const int SR_DATA_PINS[SR_TOTAL_DATA_PINS] = {4, 5, 6, 7};
// lcd functions
enum class RS_MODE { DATA, INSTRUCTION };
static FILE *lcd_stream;
static void lcd_display_set(int display_on, int cursor_on, int blink_cursor);
static void lcd_entry_mode(int increment_cursor, int shift_display);
static void lcd_function_set(int interface_mode, int number_of_lines, int fontsize);

void lcd_init() {
  SPI.begin();
  pinMode(LCD_LATCH_PIN, OUTPUT);
  digitalWrite(LCD_LATCH_PIN, LOW);
  lcd_begin();
  lcd_stream = fdevopen(lcd_putchar, NULL);
}

void lcd_write_half_command(const char c, const RS_MODE mode) {
  char sr_data = 0; // shift register data byte
  if (mode == RS_MODE::DATA) {
    sr_data |= (1 << SR_SELECT_PIN);
  }
  // D4 = least significant bit, D7 = most significant bit
  for (int i = 0; i < SR_TOTAL_DATA_PINS; i++) {  // write 4 bit command
    int pin = SR_DATA_PINS[i];
    if ((c & (1 << i)) != 0) {
      sr_data |= (1 << pin);
    }
  }
  sr_data |= (1 << SR_ENABLE_PIN);
  digitalWrite(LCD_LATCH_PIN, LOW);
  SPI.transfer(sr_data);
  digitalWrite(LCD_LATCH_PIN, HIGH);
  // keep high for tPW = 140ns
  _delay_us(1);
  digitalWrite(LCD_LATCH_PIN, LOW);
  sr_data &= ~(1 << SR_ENABLE_PIN);
  SPI.transfer(sr_data);
  digitalWrite(LCD_LATCH_PIN, HIGH);
  // minimum delay between commands is tC = 1200ns
  // since already took 1us, only need to delay for 200ns -> round to 1us
  _delay_us(1);
}

// Datasheet: https://www.openhacks.com/uploadsproductos/eone-1602a1.pdf
// Page 6 - Commands
void lcd_write_command(char c, RS_MODE mode) {
  lcd_write_half_command(c >> 4, mode);    // B[7:4]
  lcd_write_half_command(c & 0x0F, mode);    // B[3:0]
}

void lcd_begin() {
  lcd_write_half_command(0x03, RS_MODE::INSTRUCTION);
  _delay_ms(5);
  lcd_write_half_command(0x03, RS_MODE::INSTRUCTION);
  _delay_ms(11);
  lcd_write_half_command(0x03, RS_MODE::INSTRUCTION);
  lcd_write_half_command(0x02, RS_MODE::INSTRUCTION);
  lcd_function_set(4, 2, 8);
  lcd_display_set(1, 0, 0);
  lcd_entry_mode(1, 0);
  lcd_clear();
}

void lcd_clear() {
  // delay takes 1.52ms
  lcd_write_command(0x01, RS_MODE::INSTRUCTION);
  _delay_us(1520);
}

// 0,0,0,0, 1,D,C,B
// D = display on (1/0)
// C = cursor on (1/0)
// B = blink cursor (cursor position) (1/0)
void lcd_display_set(int display_on, int cursor_on, int blink_cursor) {
  char data = 0;
  data |= (1 << 3);
  if (display_on)   data |= (1 << 2);
  if (cursor_on)    data |= (1 << 1);
  if (blink_cursor) data |= (1 << 0);
  lcd_write_command(data, RS_MODE::INSTRUCTION);
  _delay_us(37);
}

// Set cursor move direction, and if display shifts
// 0,0,0,0, 0,1,ID,S
// ID = increment or decrement cursor (1/0)
// S = shift display (1/0)
void lcd_entry_mode(int increment_cursor, int shift_display) {
  char data = 0;
  data |= (1 << 2);
  if (increment_cursor) data |= (1 << 1);
  if (shift_display)    data |= (1 << 0);
  lcd_write_command(data, RS_MODE::INSTRUCTION);
  _delay_us(37);
}

// 0,0,1,DL, N,F,x,x
// DL = interface data (8/4) = 0 (4 bit)
// N = number of lines (2/1) = 1 (2 lines)
// F = font size (5x11/5x8)  = 0 (5x8 font)
void lcd_function_set(int interface_mode, int number_of_lines, int fontsize) {
  char data = 0;
  data |= (1 << 5);
  if (interface_mode == 8)  data |= (1 << 4);
  if (number_of_lines == 2) data |= (1 << 3);
  if (fontsize == 11)       data |= (1 << 2);
  lcd_write_command(data, RS_MODE::INSTRUCTION);
  _delay_us(37);
}

// This is the DDRAM address set instruction
// Page 9 - DDRAM address set
// DDRAM = display ram
// each address is selectable using 1XXX XXXX
// since 2 line mode then
// 1st row: address 0x00 to 0x27 (this assumes row size of 0x27+1 = 40)
// 2nd row: address 0x40 to 0x67
void lcd_set_cursor(char x, char y) {
  // Byte to be sent is
  char row_data = 0x00;
  if      (y == 0) row_data = 0x00; // starts at 0x00 which
  else if (y == 1) row_data = 0x40;
  else             return;
  
  char position = row_data + x;
  char command = 0x80 | (position & 0x7F); // set the MSB to write to DDRAM
  
  lcd_write_command(command, RS_MODE::INSTRUCTION);
  _delay_us(37);
}

// Set the CGRAM address to write custom characters into memory
// Page 9 - CGRAM address set
// CGRAM = character generator addresses
// CGRAM increments/decrements like DDRAM
// Has a format of 01XX XXXX
// Each character takes up 8 address spaces (E.g. 0x00 to 0x07 for char 1)
void lcd_set_custom_character(uint8_t index, const uint8_t data[LCD_CHARACTER_HEIGHT]) {
  // set CGRAM address
  uint8_t address = index*8;
  uint8_t command = 0x40 | (address & 0x3F);
  lcd_write_command(command, RS_MODE::INSTRUCTION);
  _delay_us(37);
  
  // write to CGRAM address
  // Example of image to data: https://www.8051projects.net/lcd-interfacing/lcd-custom-character.php
  for (int i = 0; i < LCD_CHARACTER_HEIGHT; i++) {
    uint8_t row_data = data[i];
    lcd_write_command(row_data, RS_MODE::DATA);
    _delay_us(37);
  }
}

int lcd_putchar(char c, FILE *fp) {
  lcd_write_command(c, RS_MODE::DATA);
  _delay_us(37);
  return 0;
}

void lcd_printf(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(lcd_stream, format, args);
  va_end(args);
}

void lcd_custom_characters_load() {
  // NOTE: max of 8 custom character
  lcd_set_custom_character(CHR_OPEN_LOCK, DATA_OPEN_LOCK);
  lcd_set_custom_character(CHR_CLOSED_LOCK, DATA_CLOSED_LOCK);
  lcd_set_custom_character(CHR_THERMOMETER, DATA_THERMOMETER_SYMBOL);
  lcd_set_custom_character(CHR_HUMIDITY, DATA_HUMIDITY_SYMBOL);
  //lcd_set_custom_character(DEGREE_CHARACTER, DEGREE_SYMBOL);
  lcd_set_custom_character(CHR_LEFT_SELECT, DATA_LEFT_SELECT_SYMBOL);
  lcd_set_custom_character(CHR_TICK, DATA_TICK_SYMBOL);
  lcd_set_custom_character(CHR_UP, DATA_UP_SYMBOL);
  lcd_set_custom_character(CHR_MAIL, DATA_MAIL_SYMBOL);
}

struct App {
  int counter = 0;
  int temperature = 22;
  int humidity = 40;
  int mail = 4;
  void step() {
    counter++;
    temperature = (temperature+12)%40+13;
    humidity = (humidity+4)%100;
    mail = (mail+3)%48;
  }
};

volatile App app;

void setup() {
  lcd_init();
  lcd_custom_characters_load();
  app = App();
}

void loop() {
  app.step();
  lcd_clear();
  lcd_set_cursor(0, 0);
  const bool is_locked = app.counter % 2 == 0;
  lcd_printf("%c Counter: %d", is_locked ? CHR_CLOSED_LOCK : CHR_OPEN_LOCK, app.counter);
  lcd_set_cursor(0, 1);
  if (is_locked) {
    lcd_printf("%c %c%dC %c%d%%", CHR_TICK, CHR_THERMOMETER, app.temperature, CHR_HUMIDITY, app.humidity);
  } else {
    lcd_printf("%c %c%d %c", CHR_LEFT_SELECT, CHR_MAIL, app.mail, CHR_UP);
  }
  delay(750);
}
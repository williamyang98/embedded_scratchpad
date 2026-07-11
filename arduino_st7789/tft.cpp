#include "./test/src/tft.hpp"
#include <avr/io.h>
#include <Arduino.h>

template <int N>
inline static void nop() {
  static_assert(N > 0);
  asm volatile("nop\n");
  nop<N-1>();
}

template <>
inline void nop<0>() {}

// Section 9.1: System function command table 1
constexpr struct {
  uint8_t NO_OPERATION = 0x00;
  uint8_t SOFTWARE_RESET = 0x01;
  uint8_t READ_DISPLAY_ID = 0x04;
  uint8_t READ_DISPLAY_STATUS = 0x09;
  uint8_t READ_DISPLAY_POWER = 0x0A;
  uint8_t READ_DISPLAY = 0x0B;
  uint8_t READ_DISPLAY_PIXEL = 0x0C;
  uint8_t READ_DISPLAY_IMAGE = 0x0D;
  uint8_t READ_DISPLAY_SIGNAL = 0x0E;
  uint8_t READ_DISPLAY_SELF_DIAGNOSTIC = 0x0F;
  uint8_t SLEEP_IN = 0x10;
  uint8_t SLEEP_OUT = 0x11;
  uint8_t PARTIAL_MODE_ON = 0x12;
  uint8_t PARTIAL_MODE_OFF = 0x13;
  uint8_t DISPLAY_INVERSION_OFF = 0x20;
  uint8_t DISPLAY_INVERSION_ON = 0x21;
  uint8_t DISPLAY_OFF = 0x28;
  uint8_t DISPLAY_ON = 0x29;
  uint8_t COLUMN_ADDRESS_SET = 0x2A;
  uint8_t ROW_ADDRESS_SET = 0x2B;
  uint8_t MEMORY_WRITE = 0x2C;
  uint8_t MEMORY_READ = 0x2E;
  uint8_t PARTIAL_ADDRESS_SET = 0x30;
  uint8_t VERTICAL_SCROLLING_DEFINITION = 0x33;
  uint8_t TEARING_EFFECT_LINE_OFF = 0x34;
  uint8_t TEARING_EFFECT_LINE_ON = 0x35;
  uint8_t MEMORY_DATA_ACCESS_CONTROL = 0x36;
  uint8_t VERTICAL_SCROLLING_START_ADDRESS = 0x37;
  uint8_t IDLE_MODE_OFF = 0x38;
  uint8_t IDLE_MODE_ON = 0x39;
  uint8_t INTERFACE_PIXEL_FORMAT = 0x3A;
  uint8_t MEMORY_WRITE_CONTINUE = 0x3C;
  uint8_t MEMORY_READ_CONTINUE = 0x3E;
  uint8_t SET_TEAR_SCANLINE = 0x44;
  uint8_t GET_SCANLINE = 0x45;
  uint8_t WRITE_DISPLAY_BRIGHTNESS = 0x51;
  uint8_t WRITE_CTRL_DISPLAY = 0x53;
  uint8_t READ_CTRL_DISPLAY = 0x54;
  uint8_t WRITE_CABC_COLOR_ENHANCEMENT = 0x55; // CABC = content adaptive brightness control
  uint8_t READ_CABC_COLOR_ENHANCEMENT = 0x56;
  uint8_t WRITE_CABC_MINIMUM_BRIGHTNESS = 0x5E;
  uint8_t READ_CABC_MINIMUM_BRIGHTNESS = 0x5F;
  uint8_t READ_AUTO_BRIGHTNESS_CONTROL_SELF_DIAGNOSTIC_RESULT = 0x68;
  uint8_t READ_ID1 = 0xDA;
  uint8_t READ_ID2 = 0xDB;
  uint8_t READ_ID3 = 0xDC;
} CMD;

// Section 8.4.1: Pin description
// 4 line serial interface I
constexpr struct {
  int8_t CHIP_SELECT = 10; // pull low to chip select 
  int8_t RESET = 9; // pull low to reset display
  int8_t DATA_COMMAND = 8; // low = command, high = data
  int8_t BACKLIGHT = 6; // high to enable backlight
  int8_t MOSI = 11; // spi tft as slave in
  int8_t SCLK = 13; // spi clock
} PIN;

namespace spi {

void init() {
  // DOC: atmel_atmega328p_datasheet.pdf
  // Section 18.5: SPI Register description
  // DORD=0: MSB first
  // MSTR=1: Master mode
  // CPOL=1: Clock high when idle
  // CPHA=0: Data sampled on leading edge of clock
  // SPI2X=1,SPR1=0,SPR0=0: f_spi = f_clock/2
  // Hardware SPI has SCLK=13, MOSI=11
  SPCR = (1 << SPE) | (0 << DORD) | (1 << MSTR) | (1 << CPOL) | (0 << CPHA) | (0 << SPR1) | (0 << SPR0);
  SPSR = (1 << SPI2X);
}


void write_byte(uint8_t data) {
  SPDR = data;
  // avoid overhead of returning value through SPI.transfer(...)
  // this polling loop is slower than a fixed number of nops
  // while (!(SPSR & (1 << SPIF))) {}
  // 8 bits, at f_spi = f_clock/2 means 16 cycles
  nop<16>();
}

void chip_select(bool is_selected) {
  digitalWrite(PIN.CHIP_SELECT, is_selected ? LOW : HIGH);
}

enum class Mode: uint8_t {
  COMMAND = 0,
  DATA = 1,
};

void set_mode(Mode mode) {
  digitalWrite(PIN.DATA_COMMAND, mode == Mode::COMMAND ? LOW : HIGH);
}

void write_command_byte(uint8_t command) {
  set_mode(Mode::COMMAND);
  chip_select(true);
  write_byte(command);
  chip_select(false);
}

void write_data_byte(uint8_t data) {
  set_mode(Mode::DATA);
  chip_select(true);
  write_byte(data);
  chip_select(false);
}

};

static struct {
  bool x_mirror = false;
  bool y_mirror = false;
} write_mode;

void tft::set_write_mode(bool x_mirror, bool y_mirror) {
  write_mode.x_mirror = x_mirror;
  write_mode.y_mirror = y_mirror;

  // Section 9.1.28: Memory data access control
  spi::write_command_byte(CMD.MEMORY_DATA_ACCESS_CONTROL);
  // D7=0: top to bottom
  // D6=0: left to right
  // D5=0: normal mode (portrait vs landscape mode)
  // D4=0: LCD refreshes from top to bottom
  // D3=0: RGB instead of BGR
  // D2=0: LCD refreshes from left to right
  // D1:D0=x: unused
  uint8_t v = 0b00000000;
  if (!x_mirror) v |= 0b01000000;
  if (!y_mirror) v |= 0b10000000;
  spi::write_data_byte(v);
}

static void cmd_set_column_address(uint16_t x_start, uint16_t x_end) {
  if (write_mode.x_mirror) {
    const uint16_t new_x_start = tft::SCREEN_WIDTH-x_end-1;
    const uint16_t new_x_end = tft::SCREEN_WIDTH-x_start-1;
    x_start = new_x_start;
    x_end = new_x_end;
  }
  // Section 9.1.20: Column address set
  spi::write_command_byte(CMD.COLUMN_ADDRESS_SET);
  spi::set_mode(spi::Mode::DATA);
  spi::chip_select(true);
  spi::write_byte(static_cast<uint8_t>(x_start >> 8));
  spi::write_byte(static_cast<uint8_t>(x_start & 0x00FF));
  spi::write_byte(static_cast<uint8_t>(x_end >> 8));
  spi::write_byte(static_cast<uint8_t>(x_end & 0x00FF));
  spi::chip_select(false);
}

static void cmd_set_row_address(uint16_t y_start, uint16_t y_end) {
  if (write_mode.y_mirror) {
    const uint16_t new_y_start = tft::SCREEN_HEIGHT-y_end-1;
    const uint16_t new_y_end = tft::SCREEN_HEIGHT-y_start-1;
    y_start = new_y_start;
    y_end = new_y_end;
  }
  // Section 9.1.21: Row address set
  y_start += tft::ADDRESS_Y_OFFSET;
  y_end += tft::ADDRESS_Y_OFFSET;
  spi::write_command_byte(CMD.ROW_ADDRESS_SET);
  spi::set_mode(spi::Mode::DATA);
  spi::chip_select(true);
  spi::write_byte(static_cast<uint8_t>(y_start >> 8));
  spi::write_byte(static_cast<uint8_t>(y_start & 0x00FF));
  spi::write_byte(static_cast<uint8_t>(y_end >> 8));
  spi::write_byte(static_cast<uint8_t>(y_end & 0x00FF));
  spi::chip_select(false);
}

// Section 8.12 Address Control 
// end point is inclusive
// end point has to be less than or equal to starting point
// address range is truncated to valid addresses meaning subsequent memory writes will be on the shrunken down rectangular address range and produce wrapping artifacts
void tft::set_write_rect(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end) {
  cmd_set_column_address(x_start, x_end);
  cmd_set_row_address(y_start, y_end);
}

void tft::init() {
  // default pin setup
  pinMode(PIN.CHIP_SELECT, OUTPUT);
  pinMode(PIN.RESET, OUTPUT);
  pinMode(PIN.DATA_COMMAND, OUTPUT);
  pinMode(PIN.BACKLIGHT, OUTPUT);
  pinMode(PIN.MOSI, OUTPUT);
  pinMode(PIN.SCLK, OUTPUT);
  digitalWrite(PIN.RESET, HIGH);
  
  spi::init();
  spi::chip_select(false);

  // default state
  hardware_reset();
  set_brightness(0);

  // init commands
  spi::write_command_byte(CMD.SOFTWARE_RESET);
  delay(150);
  spi::write_command_byte(CMD.SLEEP_OUT);
  delay(255);

  // DOC: sitronix_st7789_datasheet.pdf 
  // Section 9.1.32: Interface Pixel Format
  spi::write_command_byte(CMD.INTERFACE_PIXEL_FORMAT);
  // D6:D4=101: 65k colours
  // D2:D0=101: 16bit/pixel
  spi::write_data_byte(0b01010101); 
  // Section 8.8.3: 8-bit data bus for 16-bit/pixel (RGB 5-6-5-bit input), 65K-Colors
  delay(10);
  
  tft::set_write_mode(false, false);
  tft::set_write_rect(0, SCREEN_WIDTH-1, 0, SCREEN_HEIGHT-1);

  spi::write_command_byte(CMD.DISPLAY_INVERSION_ON);
  delay(10);
  spi::write_command_byte(CMD.PARTIAL_MODE_OFF);
  delay(10);
  spi::write_command_byte(CMD.DISPLAY_ON);
  delay(255);
}

void tft::set_brightness(uint8_t brightness) {
  analogWrite(PIN.BACKLIGHT, brightness);
}

void tft::hardware_reset() {
  digitalWrite(PIN.RESET, LOW);
  delay(50);
  digitalWrite(PIN.RESET, HIGH);
  delay(50);
}

void tft::begin_write_pixel() {
  spi::write_command_byte(CMD.MEMORY_WRITE);
  spi::set_mode(spi::Mode::DATA);
  spi::chip_select(true);
}

void tft::end_write_pixel() {
  spi::chip_select(false);
}

void tft::write_pixel(rgb565_t colour) {
  const uint8_t colour_high = static_cast<uint8_t>(colour >> 8);
  const uint8_t colour_low = static_cast<uint8_t>(colour & 0x00FF);
  spi::write_byte(colour_high);
  spi::write_byte(colour_low); 
}

void tft::fill_rect(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end, rgb565_t colour) {
  tft::set_write_rect(x_start, x_end, y_start, y_end);
  tft::begin_write_pixel();
  const uint8_t colour_high = static_cast<uint8_t>(colour >> 8);
  const uint8_t colour_low = static_cast<uint8_t>(colour & 0x00FF);
  for (uint16_t y = y_start; y <= y_end; y++) {
    for (uint16_t x = x_start; x <= x_end; x++) {
      spi::write_byte(colour_high);
      spi::write_byte(colour_low);
    }
  }
  tft::end_write_pixel();
}

void tft::fill_screen(rgb565_t colour) {
  fill_rect(0, SCREEN_WIDTH-1, 0, SCREEN_HEIGHT-1, colour);
}
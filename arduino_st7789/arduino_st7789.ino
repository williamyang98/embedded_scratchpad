#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

// DOC: sitronix_st7789_datasheet.pdf
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
} TFT_CMD;

// Section 8.8.3: 8-bit data bus for 16-bit/pixel (RGB 5-6-5-bit input), 65K-Colors
// r = 32, g = 64, b = 32, rgb = 32*64*32 = 65536
uint16_t create_tft_colour(uint8_t r, uint8_t g, uint8_t b) {
  uint16_t R = static_cast<uint16_t>(r & 0b00011111) << 11;
  uint16_t G = static_cast<uint16_t>(g & 0b00111111) << 5;
  uint16_t B = static_cast<uint16_t>(b & 0b00011111);
  uint16_t RGB = R | G | B;
  return RGB;
}

const struct {
  uint16_t BLACK    = create_tft_colour( 0, 0, 0);
  uint16_t RED      = create_tft_colour(31, 0, 0);
  uint16_t GREEN    = create_tft_colour( 0,63, 0);
  uint16_t BLUE     = create_tft_colour( 0, 0,31);
  uint16_t CYAN     = create_tft_colour( 0,63,31);
  uint16_t MAGENTA  = create_tft_colour(31, 0,31);
  uint16_t YELLOW   = create_tft_colour(31,63, 0);
  uint16_t WHITE    = create_tft_colour(31,63,31);
} TFT_COLOUR;

// Section 8.4.1: Pin description
// 4 line serial interface I
constexpr struct {
  int8_t CHIP_SELECT = 10; // pull low to chip select 
  int8_t RESET = 9; // pull low to reset display
  int8_t DATA_COMMAND = 8; // low = command, high = data
  int8_t BACKLIGHT = 6; // high to enable backlight
  int8_t MOSI = 11; // spi tft as slave in
  int8_t SCLK = 13; // spi clock
} TFT_PIN;

constexpr uint16_t TFT_WIDTH = 240;
constexpr uint16_t TFT_HEIGHT = 280;
constexpr uint16_t TFT_X_START = 0;
constexpr uint16_t TFT_X_END = TFT_X_START + TFT_WIDTH;
constexpr uint16_t TFT_Y_START = 20;
constexpr uint16_t TFT_Y_END = TFT_Y_START + TFT_HEIGHT;

static SPISettings TFT_SPI_SETTINGS;
void tft_init() {
  TFT_SPI_SETTINGS = SPISettings(8000000, MSBFIRST, SPI_MODE2);
  // default pin setup
  pinMode(TFT_PIN.CHIP_SELECT, OUTPUT);
  pinMode(TFT_PIN.RESET, OUTPUT);
  pinMode(TFT_PIN.DATA_COMMAND, OUTPUT);
  pinMode(TFT_PIN.BACKLIGHT, OUTPUT);
  pinMode(TFT_PIN.MOSI, OUTPUT);
  pinMode(TFT_PIN.SCLK, OUTPUT);
  digitalWrite(TFT_PIN.CHIP_SELECT, HIGH);
  digitalWrite(TFT_PIN.RESET, HIGH);
  digitalWrite(TFT_PIN.DATA_COMMAND, LOW);
  // init hardware spi
  SPI.begin();

  tft_hardware_reset();
  tft_set_brightness(0);

  // init commands
  tft_send_command(TFT_CMD.SOFTWARE_RESET);
  delay(150);
  tft_send_command(TFT_CMD.SLEEP_OUT);
  delay(255);
  
  // Section 9.1.32: Interface Pixel Format
  tft_send_command(TFT_CMD.INTERFACE_PIXEL_FORMAT);
  // D6:D4=101: 65k colours
  // D2:D0=101: 16bit/pixel
  tft_send_data(0b01010101); 
  // Section 8.8.3: 8-bit data bus for 16-bit/pixel (RGB 5-6-5-bit input), 65K-Colors
  delay(10);
  
  // Section 9.1.28: Memory data access control
  tft_send_command(TFT_CMD.MEMORY_DATA_ACCESS_CONTROL);
  // D7=0: top to bottom
  // D6=0: left to right
  // D5=0: normal mode
  // D4=0: LCD refreshes from top to bottom
  // D3=0: RGB instead of BGR
  // D2=0: LCD refreshes from left to right
  // D1:D0=x: unused
  tft_send_data(0b00000000);

  // Section 9.1.20: Column address set
  tft_send_command(TFT_CMD.COLUMN_ADDRESS_SET);
  tft_send_data(static_cast<uint8_t>(TFT_X_START >> 8));
  tft_send_data(static_cast<uint8_t>(TFT_X_START & 0x00FF));
  tft_send_data(static_cast<uint8_t>(TFT_X_END >> 8));
  tft_send_data(static_cast<uint8_t>(TFT_X_END & 0x00FF));

  // Section 9.1.21: Row address set
  tft_send_command(TFT_CMD.ROW_ADDRESS_SET);
  tft_send_data(static_cast<uint8_t>(TFT_Y_START >> 8));
  tft_send_data(static_cast<uint8_t>(TFT_Y_START & 0x00FF));
  tft_send_data(static_cast<uint8_t>(TFT_Y_END >> 8));
  tft_send_data(static_cast<uint8_t>(TFT_Y_END & 0x00FF));

  tft_send_command(TFT_CMD.DISPLAY_INVERSION_ON);
  delay(10);
  tft_send_command(TFT_CMD.PARTIAL_MODE_OFF);
  delay(10);
  tft_send_command(TFT_CMD.DISPLAY_ON);
  delay(255);
}

void tft_set_brightness(uint8_t brightness) {
  analogWrite(TFT_PIN.BACKLIGHT, brightness);
}

void tft_send_command(uint8_t command) {
  digitalWrite(TFT_PIN.DATA_COMMAND, LOW);
  digitalWrite(TFT_PIN.CHIP_SELECT, LOW);
  SPI.beginTransaction(TFT_SPI_SETTINGS);
  SPI.transfer(command);
  digitalWrite(TFT_PIN.CHIP_SELECT, HIGH);
  SPI.endTransaction();
}

void tft_send_data(uint8_t command) {
  digitalWrite(TFT_PIN.DATA_COMMAND, HIGH);
  digitalWrite(TFT_PIN.CHIP_SELECT, LOW);
  SPI.beginTransaction(TFT_SPI_SETTINGS);
  SPI.transfer(command);
  digitalWrite(TFT_PIN.CHIP_SELECT, HIGH);
  SPI.endTransaction();
}

void tft_hardware_reset() {
  digitalWrite(TFT_PIN.RESET, LOW);
  delay(50);
  digitalWrite(TFT_PIN.RESET, HIGH);
  delay(50);
}

void tft_fill_screen(uint16_t colour) {
    // Section 9.1.20: Column address set
  tft_send_command(TFT_CMD.COLUMN_ADDRESS_SET);
  tft_send_data(static_cast<uint8_t>(TFT_X_START >> 8));
  tft_send_data(static_cast<uint8_t>(TFT_X_START & 0x00FF));
  tft_send_data(static_cast<uint8_t>(TFT_X_END >> 8));
  tft_send_data(static_cast<uint8_t>(TFT_X_END & 0x00FF));

  // Section 9.1.21: Row address set
  tft_send_command(TFT_CMD.ROW_ADDRESS_SET);
  tft_send_data(static_cast<uint8_t>(TFT_Y_START >> 8));
  tft_send_data(static_cast<uint8_t>(TFT_Y_START & 0x00FF));
  tft_send_data(static_cast<uint8_t>(TFT_Y_END >> 8));
  tft_send_data(static_cast<uint8_t>(TFT_Y_END & 0x00FF));

  tft_send_command(TFT_CMD.MEMORY_WRITE);
  // write pixel data
  digitalWrite(TFT_PIN.DATA_COMMAND, HIGH);
  digitalWrite(TFT_PIN.CHIP_SELECT, LOW);
  SPI.beginTransaction(TFT_SPI_SETTINGS);
  const uint8_t colour_high = static_cast<uint8_t>(colour >> 8);
  const uint8_t colour_low = static_cast<uint8_t>(colour & 0x00FF);
  for (uint16_t y = 0; y < TFT_HEIGHT; y++) {
    for (uint16_t x = 0; x < TFT_WIDTH; x++) {
      SPI.transfer(colour_high);
      SPI.transfer(colour_low);
    }
  }
  digitalWrite(TFT_PIN.CHIP_SELECT, HIGH);
  SPI.endTransaction();
  // terminate memory write command
  tft_send_command(TFT_CMD.NO_OPERATION);
}

void setup(void) {
  tft_init();
  tft_set_brightness(50);
}

constexpr int TOTAL_TEST_COLOURS = 8;
static uint16_t TEST_COLOURS[TOTAL_TEST_COLOURS] = {
  TFT_COLOUR.BLACK,
  TFT_COLOUR.RED,
  TFT_COLOUR.GREEN,
  TFT_COLOUR.BLUE,
  TFT_COLOUR.CYAN,
  TFT_COLOUR.MAGENTA,
  TFT_COLOUR.YELLOW,
  TFT_COLOUR.WHITE,
};

void loop() {
  for (int i = 0; i < TOTAL_TEST_COLOURS; i++) {
    tft_fill_screen(TEST_COLOURS[i]);
    delay(500);
  }
}
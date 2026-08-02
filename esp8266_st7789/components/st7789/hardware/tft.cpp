#include "./tft.hpp"
#include "../graphics/rgb565.hpp"
#include <stdint.h>
#include <esp_err.h>
#include <driver/gpio.h>
#include <driver/spi.h>
#include <driver/pwm.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern "C" {
#include "global_pwm.h"
}

// Section 9.1: System function command table 1
static constexpr struct {
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
static constexpr struct {
    gpio_num_t CHIP_SELECT = GPIO_NUM_15; // pull low to chip select 
    gpio_num_t RESET = GPIO_NUM_5; // pull low to reset display
    gpio_num_t DATA_COMMAND = GPIO_NUM_12; // low = command, high = data
    gpio_num_t BACKLIGHT = GPIO_NUM_4; // high to enable backlight
    // hardware spi fixed pins
    gpio_num_t MOSI = GPIO_NUM_13; // spi tft as slave in
    gpio_num_t SCLK = GPIO_NUM_14; // spi clock
} PIN;

static uint8_t BACKLIGHT_PWM_CHANNEL = 0;

namespace spi {

// spi that sends 16bits at a time for pixel data
static uint32_t PIXEL_DATA_BUFFER = 0x0000;
static spi_trans_t PIXEL_TRANS_PARAMS = {
    .cmd = nullptr,
    .addr = nullptr,
    .mosi = &PIXEL_DATA_BUFFER,
    .miso = nullptr,
    .bits = {
        .mosi = sizeof(rgb565_t)*8,
    },
};

static uint32_t BYTE_COMMAND_DATA_BUFFER = 0x0000;
static spi_trans_t BYTE_COMMAND_TRANS_PARAMS = {
    .cmd = nullptr,
    .addr = nullptr,
    .mosi = &BYTE_COMMAND_DATA_BUFFER,
    .miso = nullptr,
    .bits = {
        .mosi = sizeof(uint8_t)*8,
    },
};

static void init() {
    // Hardware SPI has SCLK=D5/GPIO14, MOSI=D7/GPIO13
    spi_config_t spi_config;
    // Load default interface parameters
    spi_config.interface.val = SPI_DEFAULT_INTERFACE;
    // Load default interrupt enable
    // TRANS_DONE: true, WRITE_STATUS: false, READ_STATUS: false, WRITE_BUFFER: false, READ_BUFFER: false
    spi_config.intr_enable.val = SPI_MASTER_DEFAULT_INTR_ENABLE;
    // MSB first
    spi_config.interface.byte_tx_order = 1; // MSB first
    spi_config.interface.bit_tx_order = 0;
    spi_config.interface.mosi_en = 1;
    // Handle chip select manually on D8/GPIO15
    spi_config.interface.cs_en = 0;
    // Disable MISO pin D6/GPIO12 to free it for other GPIO operations
    spi_config.interface.miso_en = 0;
    // CPOL: 1, CPHA: 0
    spi_config.interface.cpol = 1;
    spi_config.interface.cpha = 0;
    // Set SPI to master mode
    spi_config.mode = SPI_MASTER_MODE;
    // Set the SPI clock frequency division factor
    spi_config.clk_div = SPI_80MHz_DIV;
    // Register SPI event callback function
    spi_config.event_cb = NULL;
    ESP_ERROR_CHECK(spi_init(HSPI_HOST, &spi_config));
}


static inline void write_byte(uint8_t data) {
    BYTE_COMMAND_DATA_BUFFER = uint32_t(data) << 24;
    spi_trans(HSPI_HOST, &BYTE_COMMAND_TRANS_PARAMS);
}

static void chip_select(bool is_selected) {
    ESP_ERROR_CHECK(gpio_set_level(PIN.CHIP_SELECT, is_selected ? 0 : 1));
}

enum class Mode: uint8_t {
    COMMAND = 0,
    DATA = 1,
};

static void set_mode(Mode mode) {
    ESP_ERROR_CHECK(gpio_set_level(PIN.DATA_COMMAND, mode == Mode::COMMAND ? 0 : 1));
}

static void write_command_byte(uint8_t command) {
    set_mode(Mode::COMMAND);
    chip_select(true);
    write_byte(command);
    chip_select(false);
}

static void write_data_byte(uint8_t data) {
    set_mode(Mode::DATA);
    chip_select(true);
    write_byte(data);
    chip_select(false);
}

static void write_pixel(rgb565_t pixel) {
    // Assume already set data/command and chip_select pins
    // set_mode(Mode::DATA);
    // chip_select(true);
    PIXEL_DATA_BUFFER = uint32_t(pixel) << 16;
    spi_trans(HSPI_HOST, &PIXEL_TRANS_PARAMS);
    // chip_select(false);
}

};

void tft::set_write_mode(bool x_mirror, bool y_mirror) {
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
    ESP_ERROR_CHECK(gpio_set_direction(PIN.CHIP_SELECT, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_direction(PIN.RESET, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_direction(PIN.DATA_COMMAND, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_direction(PIN.BACKLIGHT, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_direction(PIN.MOSI, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_direction(PIN.SCLK, GPIO_MODE_OUTPUT));

    // backlight pwm
    ESP_ERROR_CHECK(global_pwm_add_channel(PIN.BACKLIGHT, &BACKLIGHT_PWM_CHANNEL));

    // reset default level
    ESP_ERROR_CHECK(gpio_set_level(PIN.RESET, 1));
 
    spi::init();
    spi::chip_select(false);

    // default state
    tft::hardware_reset();
    // tft::set_brightness(0);

    // init commands
    spi::write_command_byte(CMD.SOFTWARE_RESET);
    vTaskDelay(150/portTICK_RATE_MS);
    spi::write_command_byte(CMD.SLEEP_OUT);
    vTaskDelay(250/portTICK_RATE_MS);

    // DOC: sitronix_st7789_datasheet.pdf 
    // Section 9.1.32: Interface Pixel Format
    spi::write_command_byte(CMD.INTERFACE_PIXEL_FORMAT);
    // D6:D4=101: 65k colours
    // D2:D0=101: 16bit/pixel
    spi::write_data_byte(0b01010101); 
    // Section 8.8.3: 8-bit data bus for 16-bit/pixel (RGB 5-6-5-bit input), 65K-Colors
    vTaskDelay(10/portTICK_RATE_MS);
 
    tft::set_write_mode(false, false);
    tft::set_write_rect(0, SCREEN_WIDTH-1, 0, SCREEN_HEIGHT-1);

    spi::write_command_byte(CMD.DISPLAY_INVERSION_ON);
    vTaskDelay(10/portTICK_RATE_MS);
    spi::write_command_byte(CMD.PARTIAL_MODE_OFF);
    vTaskDelay(10/portTICK_RATE_MS);
    spi::write_command_byte(CMD.DISPLAY_ON);
    vTaskDelay(250/portTICK_RATE_MS);
}

void tft::set_brightness(uint8_t brightness) {
    constexpr uint32_t SCALE = GLOBAL_PWM_PERIOD_US/256;
    const uint32_t duty_cycle = uint32_t(brightness)*SCALE;
    ESP_ERROR_CHECK(pwm_set_duty(BACKLIGHT_PWM_CHANNEL, duty_cycle));
    ESP_ERROR_CHECK(pwm_start());
}

void tft::hardware_reset() {
    ESP_ERROR_CHECK(gpio_set_level(PIN.RESET, 0));
    vTaskDelay(50/portTICK_RATE_MS);
    ESP_ERROR_CHECK(gpio_set_level(PIN.RESET, 1));
    vTaskDelay(50/portTICK_RATE_MS);
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
    spi::write_pixel(colour);
}

void tft::fill_rect(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end, rgb565_t colour) {
    tft::set_write_rect(x_start, x_end, y_start, y_end);
    tft::begin_write_pixel();
    for (uint16_t y = y_start; y <= y_end; y++) {
        for (uint16_t x = x_start; x <= x_end; x++) {
            spi::write_pixel(colour);
        }
    } 
    tft::end_write_pixel();
}

void tft::fill_screen(rgb565_t colour) {
    fill_rect(0, SCREEN_WIDTH-1, 0, SCREEN_HEIGHT-1, colour);
}
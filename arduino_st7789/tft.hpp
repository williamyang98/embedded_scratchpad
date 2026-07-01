#pragma once
#include <stdint.h>

// DOC: sitronix_st7789_datasheet.pdf
namespace tft {

constexpr uint16_t SCREEN_WIDTH = 240;
constexpr uint16_t SCREEN_HEIGHT = 280;
constexpr uint16_t ADDRESS_Y_OFFSET = 20;

typedef uint16_t rgb565_t;

// DOC: sitronix_st7789_datasheet.pdf
// Section 8.8.3: 8-bit data bus for 16-bit/pixel (RGB 5-6-5-bit input), 65K-Colors
// r = 32, g = 64, b = 32, rgb = 32*64*32 = 65536
static rgb565_t create_rgb565_bits(uint8_t r5, uint8_t g6, uint8_t b5) {
  uint16_t R = static_cast<uint16_t>(r5);
  uint16_t G = static_cast<uint16_t>(g6);
  uint16_t B = static_cast<uint16_t>(b5);
  R = (R & 0b011111) << 11;
  G = (G & 0b111111) << 5;
  B =  B & 0b011111;
  uint16_t RGB = R | G | B;
  return RGB;
}

static rgb565_t create_rgb565_u8(uint8_t r, uint8_t g, uint8_t b) {
  uint16_t R = static_cast<uint16_t>(r) * 31 / 255;
  uint16_t G = static_cast<uint16_t>(g) * 31 / 255;
  uint16_t B = static_cast<uint16_t>(b) * 31 / 255;
  R = (R & 0b011111) << 11;
  G = (G & 0b111111) << 5;
  B =  B & 0b011111;
  uint16_t RGB = R | G | B;
  return RGB;
}

static rgb565_t create_rgb565_f32(float r, float g, float b) {
  uint16_t R = static_cast<uint16_t>(r*31.0f);
  uint16_t G = static_cast<uint16_t>(g*63.0f);
  uint16_t B = static_cast<uint16_t>(b*31.0f);
  R = (R & 0b011111) << 11;
  G = (G & 0b111111) << 5;
  B =  B & 0b011111;
  uint16_t RGB = R | G | B;
  return RGB;
}

void init();
void set_brightness(uint8_t brightness);
void hardware_reset();
void fill_screen(rgb565_t colour);

void set_write_rect(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end);
void begin_write_pixel();
void write_pixel(rgb565_t colour);
void end_write_pixel();

};


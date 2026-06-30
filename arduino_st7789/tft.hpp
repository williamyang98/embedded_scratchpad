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
static rgb565_t create_rgb565_colour(uint8_t r, uint8_t g, uint8_t b) {
  uint16_t R = static_cast<uint16_t>(r & 0b00011111) << 11;
  uint16_t G = static_cast<uint16_t>(g & 0b00111111) << 5;
  uint16_t B = static_cast<uint16_t>(b & 0b00011111);
  uint16_t RGB = R | G | B;
  return RGB;
}

constexpr rgb565_t bake_rgb565_colour_f32(float r, float g, float b) {
  return 
    (static_cast<uint16_t>(r*31.0f) << 11) |
    (static_cast<uint16_t>(g*63.0f) << 5) |
     static_cast<uint16_t>(b*31.0f);
}

constexpr struct {
  rgb565_t BLACK   = bake_rgb565_colour_f32(0,0,0);
  rgb565_t RED     = bake_rgb565_colour_f32(1,0,0);
  rgb565_t GREEN   = bake_rgb565_colour_f32(0,1,0);
  rgb565_t BLUE    = bake_rgb565_colour_f32(0,0,1);
  rgb565_t CYAN    = bake_rgb565_colour_f32(0,1,1);
  rgb565_t MAGENTA = bake_rgb565_colour_f32(1,0,1);
  rgb565_t YELLOW  = bake_rgb565_colour_f32(1,1,0);
  rgb565_t WHITE   = bake_rgb565_colour_f32(1,1,1);
} COLOUR;

void init();
void set_brightness(uint8_t brightness);
void hardware_reset();
void fill_screen(rgb565_t colour);
//void set_write_rect(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end);
//void write_pixel(rgb565_t colour);

};


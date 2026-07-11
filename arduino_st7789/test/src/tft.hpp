#pragma once
#include <stdint.h>
#include "./rgb565.hpp"

// DOC: sitronix_st7789_datasheet.pdf
namespace tft {

constexpr uint16_t SCREEN_WIDTH = 240;
constexpr uint16_t SCREEN_HEIGHT = 280;
constexpr uint16_t ADDRESS_Y_OFFSET = 20;

void init();
void set_brightness(uint8_t brightness);
void hardware_reset();
void set_write_mode(bool x_mirror, bool y_mirror);
void set_write_rect(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end);
void begin_write_pixel();
void write_pixel(rgb565_t colour);
void end_write_pixel();
void fill_rect(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end, rgb565_t colour);
void fill_screen(rgb565_t colour);

};

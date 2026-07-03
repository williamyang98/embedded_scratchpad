#pragma once
#include "./st7789.hpp"

namespace gfx {

bool write_digit(ST7789& screen, char digit, uint16_t x_start, uint16_t y_start, rgb565_t text_colour, rgb565_t background_colour);

};

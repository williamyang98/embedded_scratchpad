#pragma once
#include <stdint.h>
#include "./tft.hpp"

struct Glyph;

namespace gfx {

void write_digit(const Glyph& glyph, uint16_t x_start, uint16_t y_start, tft::rgb565_t text_colour, tft::rgb565_t background_colour);

};
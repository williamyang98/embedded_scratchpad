#pragma once
#include "./st7789.hpp"
#include "../scripts/glyphs/glyph.hpp"

namespace gfx {

void write_glyph(ST7789& screen, const glyph::Glyph& glyph, uint16_t x_start, uint16_t y_start, rgb565_t text_colour, rgb565_t background_colour);

};

#pragma once
#include "./st7789.hpp"

namespace gfx {

void fill_screen(ST7789& screen, rgb565_t colour);
void fill_rect(ST7789& screen, Rect rect, rgb565_t colour);

};

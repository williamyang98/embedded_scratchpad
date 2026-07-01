#include "./gfx.hpp"

namespace gfx {

void fill_screen(ST7789& screen, rgb565_t colour) {
    const auto image = screen.image();
    const Rect rect = {
        .x_start = 0,
        .x_end = uint16_t(image.width()-1),
        .y_start = 0,
        .y_end = uint16_t(image.height()-1),
    };
    const uint32_t total_pixels = rect.size();
    screen.set_write_rect(rect);
    for (uint32_t i = 0; i < total_pixels; i++) {
        screen.write(colour);
    }
}

void fill_rect(ST7789& screen, Rect rect, rgb565_t colour) {
    screen.set_write_rect(rect);
    const uint32_t total_pixels = rect.size();
    for (uint32_t i = 0; i < total_pixels; i++) {
        screen.write(colour);
    }
}

};

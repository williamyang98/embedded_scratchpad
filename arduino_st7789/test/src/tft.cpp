#include "./tft.hpp"
#include "./st7789.hpp"

namespace tft {

void init() {
    st7789 = std::make_unique<ST7789>(SCREEN_WIDTH, SCREEN_HEIGHT);
}

void set_brightness(uint8_t brightness) {
    st7789->set_brightness(brightness);
}

void hardware_reset() {
    st7789->hardware_reset();
}

void fill_screen(rgb565_t colour) {
    tft::set_write_rect(0, SCREEN_WIDTH-1, 0, SCREEN_HEIGHT-1);
    tft::begin_write_pixel();
    for (uint16_t y = 0; y < SCREEN_HEIGHT; y++) {
        for (uint16_t x = 0; x < SCREEN_WIDTH; x++) {
            write_pixel(colour);
        }
    }
    tft::end_write_pixel();
}

void set_write_rect(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end) {
    st7789->set_write_rect({
        .x_start = x_start,
        .x_end = x_end,
        .y_start = y_start,
        .y_end = y_end,
    });
}

void begin_write_pixel() {
    st7789->begin_write_pixel();
}

void write_pixel(rgb565_t colour) {
    st7789->write(colour);
}

void end_write_pixel() {
    st7789->end_write_pixel();
}

};

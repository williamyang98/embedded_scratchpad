#include "./tft.hpp"
#include "./st7789.hpp"

namespace tft {

void init() {

}

void set_brightness(uint8_t brightness) {
    g_st7789.set_brightness(brightness);
}

void hardware_reset() {
    g_st7789.hardware_reset();
}

void set_write_mode(bool x_mirror, bool y_mirror) {
    g_st7789.set_write_mode(x_mirror, y_mirror);
}

void set_write_rect(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end) {
    g_st7789.set_write_rect({
        .x_start = x_start,
        .x_end = x_end,
        .y_start = y_start,
        .y_end = y_end,
    });
}

void begin_write_pixel() {
    g_st7789.begin_write_pixel();
}

void write_pixel(rgb565_t colour) {
    g_st7789.write(colour);
}

void end_write_pixel() {
    g_st7789.end_write_pixel();
}

void fill_rect(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end, rgb565_t colour) {
    tft::set_write_rect(x_start, x_end, y_start, y_end);
    tft::begin_write_pixel();
    for (uint16_t y = y_start; y <= y_end; y++) {
        for (uint16_t x = x_start; x <= x_end; x++) {
            write_pixel(colour);
        }
    }
    tft::end_write_pixel();
}

void fill_screen(rgb565_t colour) {
    fill_rect(0, SCREEN_WIDTH-1, 0, SCREEN_HEIGHT-1, colour);
}

};

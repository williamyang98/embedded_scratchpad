#include "./tft.hpp"
#include "./st7789.hpp"

namespace tft {

void init() {

}

void set_brightness(uint8_t brightness) {
    st7789.set_brightness(brightness);
}

void hardware_reset() {
    st7789.hardware_reset();
}

static struct {
    bool x_mirror;
    bool y_mirror;
} write_mode;

void set_write_mode(bool x_mirror, bool y_mirror) {
    st7789.set_write_mode(x_mirror, y_mirror);
    write_mode.x_mirror = x_mirror;
    write_mode.y_mirror = y_mirror;
}

void set_write_rect(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end) {
    // DOC: sitronix_st7789_datasheet.pdf
    // Section 8.11.2: Memory to display address mapping
    if (write_mode.x_mirror) {
        const uint16_t mirror_x_start = SCREEN_WIDTH-x_end-1;
        const uint16_t mirror_x_end = SCREEN_WIDTH-x_start-1;
        x_start = mirror_x_start;
        x_end = mirror_x_end;
    }
    if (write_mode.y_mirror) {
        const uint16_t mirror_y_start = SCREEN_HEIGHT-y_end-1;
        const uint16_t mirror_y_end = SCREEN_HEIGHT-y_start-1;
        y_start = mirror_y_start;
        y_end = mirror_y_end;
    }
    st7789.set_write_rect({
        .x_start = x_start,
        .x_end = x_end,
        .y_start = y_start,
        .y_end = y_end,
    });
}

void begin_write_pixel() {
    st7789.begin_write_pixel();
}

void write_pixel(rgb565_t colour) {
    st7789.write(colour);
}

void end_write_pixel() {
    st7789.end_write_pixel();
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

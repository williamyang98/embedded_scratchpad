#pragma once
#include "./rgb565.hpp"
#include "./tft.hpp"

struct SolidBackgroundColour {
    rgb565_t colour;
    SolidBackgroundColour(rgb565_t _colour): colour(_colour) {}
    inline rgb565_t next_colour() const { return colour; }
};

class RadialBackgroundColour {
public:
    rgb565_t background_colour;
    rgb565_t delta_colour;
    uint16_t x_start;
    uint16_t x_end;
    uint16_t y_start;
    uint16_t y_end;
    uint16_t x;
    uint16_t y;
public:
    void reset_cursor() {
        x = x_start;
        y = y_start;
    }
    rgb565_t next_colour() {
        const rgb565_t colour = get_colour();
        x += 1;
        if (x > x_end) {
            x = x_start;
            y += 1;
        }
        // Assume no wrap around
        // if (y >= y_end) {
        //     y = y_start;
        // }
        return colour;
    }
    rgb565_t get_colour() const {
        const uint16_t distance = get_alpha_max_plus_beta_min_distance();
        const uint16_t scale = distance >> 5;
        const rgb565_t colour = delta_colour*scale + background_colour;
        return colour;
    }
    void fill() {
        tft::set_write_rect(x_start, x_end, y_start, y_end);
        tft::begin_write_pixel();
        for (y = y_start; y <= y_end; y++) {
            for (x = x_start; x <= x_end; x++) {
                const rgb565_t colour = get_colour();    
                tft::write_pixel(colour);
            }
        }
        tft::end_write_pixel();
    }
private:
    // https://en.wikipedia.org/wiki/Alpha_max_plus_beta_min_algorithm
    // alpha = 1/1, beta = 3/8 for good approximation
    inline uint16_t get_alpha_max_plus_beta_min_distance() const {
        uint16_t a = x;
        uint16_t b = y;
        if (a < b) {
            const uint16_t temp = a;
            a = b;
            b = temp;
        }
        return a + 3*b/8;
    }
};

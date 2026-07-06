#pragma once
#include <stdint.h>
#include "./rgb565.hpp"
#include "./font.hpp"

struct RightToLeftPrinter {
    uint16_t x_end;
    uint16_t y_end;
    rgb565_t text_colour;

    template <typename G, typename F>
    bool print_char(uint8_t c, F background_colour_source, G glyph_source) {
        const glyph::Glyph* glyph = glyph_source(c);
        if (glyph == nullptr) return false;
        if (x_end+1 < glyph->width) return false;
        if (y_end+1 < glyph->height) return false;

        const uint16_t x_start = x_end-glyph->width+1;
        const uint16_t y_start = y_end-glyph->height+1;

        background_colour_source.x_start = x_start;
        background_colour_source.y_start = y_start;
        background_colour_source.x_end = x_end;
        background_colour_source.y_end = y_end;
        background_colour_source.reset_cursor();
        write_glyph(*glyph, x_start, y_start, text_colour, background_colour_source);
        x_end = x_start-1;

        return true;
    }

    template <typename G, typename F>
    void print_string(const char* str, F background_colour_source, G glyph_source) {
        int16_t N = 0;
        while (str[N] != 0) {
            N++;
        }
        for (int16_t i = N-1; i >= 0; i--) {
            print_char(str[i], background_colour_source, glyph_source);
        }
    };

    template <typename F>
    bool pad_nothing(uint16_t width, uint16_t height, F background_colour_source) {
        if (x_end+1 < width) return false;
        if (y_end+1 < height) return false;

        const uint16_t x_start = x_end-width+1;
        const uint16_t y_start = y_end-height+1;

        background_colour_source.x_start = x_start;
        background_colour_source.y_start = y_start;
        background_colour_source.x_end = x_end;
        background_colour_source.y_end = y_end;
        background_colour_source.reset_cursor();
        background_colour_source.fill();
        x_end = x_start-1;

        return true;
    }
};

struct LeftToRightPrinter {
    uint16_t x_start;
    uint16_t y_end;
    rgb565_t text_colour;

    template <typename G, typename F>
    bool print_char(uint8_t c, F background_colour_source, G glyph_source) {
        const glyph::Glyph* glyph = glyph_source(c);
        if (glyph == nullptr) return false;
        const uint16_t x_end = x_start+glyph->width-1;
        if (x_end >= tft::SCREEN_WIDTH) return false;
        if (y_end+1 < glyph->height) return false;
        const uint16_t y_start = y_end-glyph->height+1;

        background_colour_source.x_start = x_start;
        background_colour_source.y_start = y_start;
        background_colour_source.x_end = x_end;
        background_colour_source.y_end = y_end;
        background_colour_source.reset_cursor();
        write_glyph(*glyph, x_start, y_start, text_colour, background_colour_source);
        x_start = x_end+1;

        return true;
    }

    template <typename G, typename F>
    void print_string(const char* str, F background_colour_source, G glyph_source) {
        int16_t i = 0;
        while (str[i] != 0) {
            print_char(str[i], background_colour_source, glyph_source);
            i++;
        }
    };

    template <typename F>
    bool pad_nothing(uint16_t width, uint16_t height, F background_colour_source) {
        const uint16_t x_end = x_start+width-1;
        if (x_end >= tft::SCREEN_WIDTH) return false;
        if (y_end+1 < height) return false;
        const uint16_t y_start = y_end-height+1;

        background_colour_source.x_start = x_start;
        background_colour_source.y_start = y_start;
        background_colour_source.x_end = x_end;
        background_colour_source.y_end = y_end;
        background_colour_source.reset_cursor();
        background_colour_source.fill();
        x_start = x_end+1;

        return true;
    }
};

struct Digits {
    int16_t value;
    uint16_t absolute_value;
    static constexpr uint8_t TOTAL_DIGITS = 5;
    uint8_t digits[5];
    uint8_t leading_non_zero_digit_index;
    bool is_minus;
    Digits(int16_t _value) {
        value = _value;
        is_minus = value < 0;
        absolute_value = value & ~(1 << 15);
        leading_non_zero_digit_index = 0;
        uint16_t counter = absolute_value;
        for (uint8_t i = 0; i < TOTAL_DIGITS; i++) {
            const uint8_t digit = counter % 10;
            if (digit > 0) {
                leading_non_zero_digit_index = i;
            }
            digits[i] = digit;
            counter = (counter-digit) / 10;
        }
    }
    uint8_t operator[](uint8_t i) const {
        return digits[i];
    }
};


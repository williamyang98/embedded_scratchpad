#pragma once
#include <stdint.h>
#include "./bit_reader.hpp"
#include "./tft.hpp"
#include "./rgb565.hpp"
#include "../scripts/glyphs/glyph.hpp"

template <typename F>
static void write_glyph_grayscale_rle_u4(const glyph::Glyph& glyph, uint16_t x_start, uint16_t y_start, rgb565_t text_colour, F background_colour_source) {
    const uint16_t width = glyph.width;
    const uint16_t height = glyph.height;
    const uint16_t x_end = static_cast<uint16_t>(x_start+width-1);
    const uint16_t y_end = static_cast<uint16_t>(y_start+height-1);
    tft::set_write_rect(x_start, x_end, y_start, y_end);

    const uint16_t total_pixels = width*height;
    uint16_t curr_pixel = 0;
    BitReader bit_reader(glyph.data);

    tft::begin_write_pixel();
    while (curr_pixel < total_pixels) {
        const uint8_t value = bit_reader.read_bits(2);
        if (value == 0 || value == 3) {
            const uint8_t length = bit_reader.read_bits(8);
            for (uint8_t i = 0; i < length; i++) {
                const rgb565_t background_colour = background_colour_source.next_colour();
                const rgb565_t colour = blend_rgb565_u4(text_colour, background_colour, value);
                tft::write_pixel(colour);
            }
            curr_pixel += static_cast<uint16_t>(length);
        } else {
            const rgb565_t background_colour = background_colour_source.next_colour();
            const rgb565_t colour = blend_rgb565_u4(text_colour, background_colour, value);
            tft::write_pixel(colour);
            curr_pixel += 1;
        }
    }
    tft::end_write_pixel();
}

template <typename F>
static void write_glyph_binary_rle_u8(const glyph::Glyph& glyph, uint16_t x_start, uint16_t y_start, rgb565_t text_colour, F background_colour_source) {
    const uint16_t width = glyph.width;
    const uint16_t height = glyph.height;
    const uint16_t x_end = static_cast<uint16_t>(x_start+width-1);
    const uint16_t y_end = static_cast<uint16_t>(y_start+height-1);
    tft::set_write_rect(x_start, x_end, y_start, y_end);

    tft::begin_write_pixel();
    constexpr uint8_t mask = 0b10000000;
    const uint16_t total_data = glyph.data_length;
    for (uint16_t curr_data = 0; curr_data < total_data; curr_data++) {
        const uint8_t data = pgm_read_byte(glyph.data + curr_data);
        const uint8_t pixel = data & mask;
        const uint8_t length = data & ~mask;
        if (pixel == 0) {
            for (uint8_t i = 0; i < length; i++) {
                const rgb565_t background_colour = background_colour_source.next_colour();
                tft::write_pixel(background_colour);
            }
        } else {
            for (uint8_t i = 0; i < length; i++) {
                tft::write_pixel(text_colour);
            }
        }
    }
    tft::end_write_pixel();
}

template <typename F>
void write_glyph(
    const glyph::Glyph& glyph,
    uint16_t x_start, uint16_t y_start,
    rgb565_t text_colour, F background_colour_source
) {
    switch (glyph.encoding) {
    case glyph::Encoding::BINARY_RLE_U8: return write_glyph_binary_rle_u8<F>(glyph, x_start, y_start, text_colour, background_colour_source);
    case glyph::Encoding::GRAYSCALE_RLE_U4: return write_glyph_grayscale_rle_u4<F>(glyph, x_start, y_start, text_colour, background_colour_source);
    default: break;
    }
}

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


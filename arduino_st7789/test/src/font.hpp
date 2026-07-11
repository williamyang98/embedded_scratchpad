#pragma once
#include <stdint.h>
#include "./bit_reader.hpp"
#include "./tft.hpp"
#include "./rgb565.hpp"
#include "./glyph.hpp"
#include "../scripts/glyphs/icons.hpp"

template <typename F>
static void write_glyph_grayscale_rle_q4(const glyph::Glyph& glyph, uint16_t x_start, uint16_t y_start, rgb565_t text_colour, F background_colour_source) {
    const uint16_t width = glyph.width;
    const uint16_t height = glyph.height;
    const uint16_t x_end = static_cast<uint16_t>(x_start+width-1);
    const uint16_t y_end = static_cast<uint16_t>(y_start+height-1);
    background_colour_source.x_start = x_start;
    background_colour_source.x_end = x_end;
    background_colour_source.y_start = y_start;
    background_colour_source.y_end = y_end;
    background_colour_source.reset_cursor();

    const uint16_t total_pixels = width*height;
    uint16_t curr_pixel = 0;
    BitReader bit_reader(glyph.data);

    tft::set_write_rect(x_start, x_end, y_start, y_end);
    tft::begin_write_pixel();
    while (curr_pixel < total_pixels) {
        const uint8_t pixel = bit_reader.read_bits(2);
        if (pixel == 0) {
            const uint8_t length = bit_reader.read_bits(8);
            for (uint8_t i = 0; i < length; i++) {
                const rgb565_t background_colour = background_colour_source.get_colour();
                background_colour_source.advance_cursor();
                tft::write_pixel(background_colour);
            }
            curr_pixel += static_cast<uint16_t>(length);
        } else if (pixel == 3) {
            const uint8_t length = bit_reader.read_bits(8);
            for (uint8_t i = 0; i < length; i++) {
                background_colour_source.advance_cursor();
                tft::write_pixel(text_colour);
            }
            curr_pixel += static_cast<uint16_t>(length);
        } else {
            const rgb565_t background_colour = background_colour_source.get_colour();
            const rgb565_t colour = blend_rgb565_u4(text_colour, background_colour, pixel);
            background_colour_source.advance_cursor();
            tft::write_pixel(colour);
            curr_pixel += 1;
        }
    }
    tft::end_write_pixel();
}

template <typename F>
static void write_glyph_binary_rle_q1(const glyph::Glyph& glyph, uint16_t x_start, uint16_t y_start, rgb565_t text_colour, F background_colour_source) {
    const uint16_t width = glyph.width;
    const uint16_t height = glyph.height;
    const uint16_t x_end = static_cast<uint16_t>(x_start+width-1);
    const uint16_t y_end = static_cast<uint16_t>(y_start+height-1);
    background_colour_source.x_start = x_start;
    background_colour_source.x_end = x_end;
    background_colour_source.y_start = y_start;
    background_colour_source.y_end = y_end;
    background_colour_source.reset_cursor();

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
                const rgb565_t background_colour = background_colour_source.get_colour();
                background_colour_source.advance_cursor();
                tft::write_pixel(background_colour);
            }
        } else {
            for (uint8_t i = 0; i < length; i++) {
                background_colour_source.advance_cursor();
                tft::write_pixel(text_colour);
            }
        }
    }
    tft::end_write_pixel();
}

template <typename F>
static void write_glyph_binary_q1(const glyph::Glyph& glyph, uint16_t x_start, uint16_t y_start, rgb565_t text_colour, F background_colour_source) {
    const uint8_t width = glyph.width;
    const uint8_t height = glyph.height;
    const uint16_t x_end = static_cast<uint16_t>(x_start+uint16_t(width)-1);
    const uint16_t y_end = static_cast<uint16_t>(y_start+uint16_t(height)-1);
    background_colour_source.x_start = x_start;
    background_colour_source.x_end = x_end;
    background_colour_source.y_start = y_start;
    background_colour_source.y_end = y_end;
    background_colour_source.reset_cursor();

    tft::set_write_rect(x_start, x_end, y_start, y_end);
    tft::begin_write_pixel();
    uint8_t data = pgm_read_byte(glyph.data + 0);
    uint8_t curr_bit = 0;
    uint16_t curr_byte = 0;
    for (uint8_t y = 0; y < height; y++) {
        for (uint8_t x = 0; x < width; x++) {
            if (curr_bit == 8) {
                curr_bit = 0;
                curr_byte++;
                data = pgm_read_byte(glyph.data + curr_byte);
            }
            const uint8_t pixel = data & 0b1;
            data = data >> 1;
            curr_bit++;
            if (pixel == 0) {
                const rgb565_t background_colour = background_colour_source.get_colour();
                background_colour_source.advance_cursor();
                tft::write_pixel(background_colour);
            } else {
                background_colour_source.advance_cursor();
                tft::write_pixel(text_colour);
            }
        }
    }
    tft::end_write_pixel();
}

template <typename F>
static void write_glyph_grayscale_q4(const glyph::Glyph& glyph, uint16_t x_start, uint16_t y_start, rgb565_t text_colour, F background_colour_source) {
    const uint8_t width = glyph.width;
    const uint8_t height = glyph.height;
    const uint16_t x_end = static_cast<uint16_t>(x_start+uint16_t(width)-1);
    const uint16_t y_end = static_cast<uint16_t>(y_start+uint16_t(height)-1);
    background_colour_source.x_start = x_start;
    background_colour_source.x_end = x_end;
    background_colour_source.y_start = y_start;
    background_colour_source.y_end = y_end;
    background_colour_source.reset_cursor();

    tft::set_write_rect(x_start, x_end, y_start, y_end);
    tft::begin_write_pixel();
    uint8_t data = pgm_read_byte(glyph.data + 0);
    uint8_t curr_bit = 0;
    uint16_t curr_byte = 0;
    for (uint8_t y = 0; y < height; y++) {
        for (uint8_t x = 0; x < width; x++) {
            if (curr_bit == 8) {
                curr_bit = 0;
                curr_byte++;
                data = pgm_read_byte(glyph.data + curr_byte);
            }
            const uint8_t pixel = data & 0b11;
            data = data >> 2;
            curr_bit+=2;
            if (pixel == 0) {
                const rgb565_t background_colour = background_colour_source.get_colour();
                background_colour_source.advance_cursor();
                tft::write_pixel(background_colour);
            } else if (pixel == 3) {
                background_colour_source.advance_cursor();
                tft::write_pixel(text_colour);
            } else {
                const rgb565_t background_colour = background_colour_source.get_colour();
                const rgb565_t colour = blend_rgb565_u4(text_colour, background_colour, pixel);
                background_colour_source.advance_cursor();
                tft::write_pixel(colour);
            }
        }
    }
    tft::end_write_pixel();
}

extern struct GlyphRGBAQ256PaletteRenderSettings {
    bool x_mirror;
    bool y_mirror;
} glyph_rgba_q256_palette_render_settings;

template <typename F>
static void write_glyph_rgba_q256_palette(const glyph::Glyph& glyph, uint16_t x_start, uint16_t y_start, rgb565_t text_colour, F background_colour_source) {
    const uint8_t width = glyph.width;
    const uint8_t height = glyph.height;
    const uint16_t x_end = static_cast<uint16_t>(x_start+uint16_t(width)-1);
    const uint16_t y_end = static_cast<uint16_t>(y_start+uint16_t(height)-1);
    background_colour_source.x_start = x_start;
    background_colour_source.x_end = x_end;
    background_colour_source.y_start = y_start;
    background_colour_source.y_end = y_end;
    background_colour_source.reset_cursor();

    tft::set_write_rect(x_start, x_end, y_start, y_end);
    tft::begin_write_pixel();

    #define push_pixel(palette_index) { \
        if (palette_index == icons::TRANSPARENCY_PALETTE_VALUE) { \
            const rgb565_t background_colour = background_colour_source.get_colour(); \
            background_colour_source.advance_cursor(); \
            tft::write_pixel(background_colour); \
        } else if (palette_index == icons::CHROMA_KEY_PALETTE_VALUE) { \
            background_colour_source.advance_cursor(); \
            tft::write_pixel(text_colour); \
        } else { \
            const rgb565_t icon_colour = pgm_read_word(icons::RGB565_COLOUR_PALETTE + palette_index); \
            background_colour_source.advance_cursor(); \
            tft::write_pixel(icon_colour); \
        } \
    }

    const auto& settings = glyph_rgba_q256_palette_render_settings;
    if (!settings.x_mirror && !settings.y_mirror) {
        uint16_t i = 0;
        for (uint8_t y = 0; y < height; y++) {
            for (uint8_t x = 0; x < width; x++) {
                const uint8_t palette_index = pgm_read_byte(glyph.data + i);
                push_pixel(palette_index);
                i++;
            }
        }
    } else if (settings.x_mirror && !settings.y_mirror) {
        uint16_t i = width-1;
        for (uint8_t y = 0; y < height; y++) {
            for (uint8_t x = 0; x < width; x++) {
                const uint8_t palette_index = pgm_read_byte(glyph.data + i - x);
                push_pixel(palette_index);
            }
            i += width;
        }
    } else if (!settings.x_mirror && settings.y_mirror) {
        uint16_t i = static_cast<uint16_t>(width)*static_cast<uint16_t>(height-1);
        for (uint8_t y = 0; y < height; y++) {
            for (uint8_t x = 0; x < width; x++) {
                const uint8_t palette_index = pgm_read_byte(glyph.data + i + x);
                push_pixel(palette_index);
            }
            i -= width;
        }
    } else {
        uint16_t i = (static_cast<uint16_t>(width)*static_cast<uint16_t>(height))-1;
        for (uint8_t y = 0; y < height; y++) {
            for (uint8_t x = 0; x < width; x++) {
                const uint8_t palette_index = pgm_read_byte(glyph.data + i);
                push_pixel(palette_index);
                i--;
            }
        }
    }

    #undef push_pixel
    tft::end_write_pixel();
}

template <typename F>
void write_glyph(
    const glyph::Glyph& glyph,
    uint16_t x_start, uint16_t y_start,
    rgb565_t text_colour, F background_colour_source
) {
    switch (glyph.encoding) {
    case glyph::Encoding::BINARY_RLE_Q1: return write_glyph_binary_rle_q1(glyph, x_start, y_start, text_colour, background_colour_source);
    case glyph::Encoding::GRAYSCALE_RLE_Q4: return write_glyph_grayscale_rle_q4(glyph, x_start, y_start, text_colour, background_colour_source);
    case glyph::Encoding::BINARY_Q1: return write_glyph_binary_q1(glyph, x_start, y_start, text_colour, background_colour_source);
    case glyph::Encoding::GRAYSCALE_Q4: return write_glyph_grayscale_q4(glyph, x_start, y_start, text_colour, background_colour_source);
    case glyph::Encoding::RGBA_Q256_PALETTE: return write_glyph_rgba_q256_palette(glyph, x_start, y_start, text_colour, background_colour_source);
    default: break;
    }
}


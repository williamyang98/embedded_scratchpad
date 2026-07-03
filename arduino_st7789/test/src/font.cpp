#include "./font.hpp"
#include <stdint.h>
#include <span>
#include "./bit_reader.hpp"

namespace gfx {

static rgb565_t blend_rgb565_u4(rgb565_t colour_0, rgb565_t colour_1, uint8_t alpha) {
    const uint8_t r_0 = (colour_0 & 0b11111'000000'00000) >> 11;
    const uint8_t g_0 = (colour_0 & 0b00000'111111'00000) >> 5;
    const uint8_t b_0 =  colour_0 & 0b00000'000000'11111;
    const uint8_t r_1 = (colour_1 & 0b11111'000000'00000) >> 11;
    const uint8_t g_1 = (colour_1 & 0b00000'111111'00000) >> 5;
    const uint8_t b_1 =  colour_1 & 0b00000'000000'11111;

    const uint8_t beta = 3-alpha;
    const uint8_t r = (r_0*alpha + r_1*beta)/3;
    const uint8_t g = (g_0*alpha + g_1*beta)/3;
    const uint8_t b = (b_0*alpha + b_1*beta)/3;

    const uint16_t R = (static_cast<uint16_t>(r) & 0b011111) << 11;
    const uint16_t G = (static_cast<uint16_t>(g) & 0b111111) << 5;
    const uint16_t B =  static_cast<uint16_t>(b) & 0b011111;
    return R | G | B;
}

static void write_glyph_grayscale_rle_u4(
    ST7789& screen, const glyph::Glyph& glyph,
    uint16_t x_start, uint16_t y_start,
    rgb565_t text_colour, rgb565_t background_colour
) {
    const uint16_t width = glyph.width;
    const uint16_t height = glyph.height;

    const Rect rect = {
        .x_start = x_start,
        .x_end = static_cast<uint16_t>(x_start+width-1),
        .y_start = y_start,
        .y_end = static_cast<uint16_t>(y_start+height-1),
    };

    const uint16_t total_pixels = width*height;
    screen.set_write_rect(rect);

    uint16_t curr_pixel = 0;
    auto bit_reader = BitReader(std::span(glyph.data, glyph.data_length));

    while (curr_pixel < total_pixels) {
        const uint8_t value = bit_reader.read_bits(2);
        const rgb565_t colour = blend_rgb565_u4(text_colour, background_colour, value);
        if (value == 0 || value == 3) {
            const uint8_t length = bit_reader.read_bits(8);
            for (uint16_t i = 0; i < length; i++) {
                screen.write(colour);
            }
            curr_pixel += static_cast<uint16_t>(length);
        } else {
            curr_pixel += 1;
            screen.write(colour);
        }
    }
}

static void write_glyph_binary_rle_u8(
    ST7789& screen, const glyph::Glyph& glyph,
    uint16_t x_start, uint16_t y_start,
    rgb565_t text_colour, rgb565_t background_colour
) {
    const uint16_t width = glyph.width;
    const uint16_t height = glyph.height;
    const Rect rect = {
        .x_start = x_start,
        .x_end = static_cast<uint16_t>(x_start+width-1),
        .y_start = y_start,
        .y_end = static_cast<uint16_t>(y_start+height-1),
    };
    screen.set_write_rect(rect);
    const uint8_t mask = 0b10000000;
;   for (uint16_t i = 0; i < glyph.data_length; i++) {
        const uint8_t rle = glyph.data[i];
        const uint8_t value = rle & mask;
        const uint8_t length = rle & ~mask;
        const rgb565_t colour = value == 0 ? background_colour : text_colour;
        for (uint8_t j = 0; j != length; j++) {
            screen.write(colour);
        }
    }
}

void write_glyph(
    ST7789& screen, const glyph::Glyph& glyph,
    uint16_t x_start, uint16_t y_start,
    rgb565_t text_colour, rgb565_t background_colour
) {
    switch (glyph.encoding) {
    case glyph::Encoding::BINARY_RLE_U8: return write_glyph_binary_rle_u8(screen, glyph, x_start, y_start, text_colour, background_colour);
    case glyph::Encoding::GRAYSCALE_RLE_U4: return write_glyph_grayscale_rle_u4(screen, glyph, x_start, y_start, text_colour, background_colour);
    default: break;
    }
}

};

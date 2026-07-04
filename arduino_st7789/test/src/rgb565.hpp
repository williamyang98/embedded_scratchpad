#pragma once
#include <stdint.h>

typedef uint16_t rgb565_t;

// DOC: sitronix_st7789_datasheet.pdf
// Section 8.8.3: 8-bit data bus for 16-bit/pixel (RGB 5-6-5-bit input), 65K-Colors
// r = 32, g = 64, b = 32, rgb = 32*64*32 = 65536
static rgb565_t create_rgb565_bits(uint8_t r5, uint8_t g6, uint8_t b5) {
    uint16_t R = static_cast<uint16_t>(r5);
    uint16_t G = static_cast<uint16_t>(g6);
    uint16_t B = static_cast<uint16_t>(b5);
    R = (R & 0b011111) << 11;
    G = (G & 0b111111) << 5;
    B =  B & 0b011111;
    uint16_t RGB = R | G | B;
    return RGB;
}

static rgb565_t create_rgb565_u8(uint8_t r, uint8_t g, uint8_t b) {
    uint16_t R = static_cast<uint16_t>(r) * 31 / 255;
    uint16_t G = static_cast<uint16_t>(g) * 31 / 255;
    uint16_t B = static_cast<uint16_t>(b) * 31 / 255;
    R = (R & 0b011111) << 11;
    G = (G & 0b111111) << 5;
    B =  B & 0b011111;
    uint16_t RGB = R | G | B;
    return RGB;
}

static rgb565_t create_rgb565_f32(float r, float g, float b) {
    uint16_t R = static_cast<uint16_t>(r*31.0f);
    uint16_t G = static_cast<uint16_t>(g*63.0f);
    uint16_t B = static_cast<uint16_t>(b*31.0f);
    R = (R & 0b011111) << 11;
    G = (G & 0b111111) << 5;
    B =  B & 0b011111;
    uint16_t RGB = R | G | B;
    return RGB;
}

static rgb565_t blend_rgb565_u4(rgb565_t colour_0, rgb565_t colour_1, uint8_t alpha) {
    const uint8_t r_0 = (colour_0 & 0b1111100000000000) >> 11;
    const uint8_t g_0 = (colour_0 & 0b0000011111100000) >> 5;
    const uint8_t b_0 =  colour_0 & 0b0000000000011111;
    const uint8_t r_1 = (colour_1 & 0b1111100000000000) >> 11;
    const uint8_t g_1 = (colour_1 & 0b0000011111100000) >> 5;
    const uint8_t b_1 =  colour_1 & 0b0000000000011111;

    const uint8_t beta = 3-alpha;
    const uint8_t r = (r_0*alpha + r_1*beta)/3;
    const uint8_t g = (g_0*alpha + g_1*beta)/3;
    const uint8_t b = (b_0*alpha + b_1*beta)/3;

    const uint16_t R = (static_cast<uint16_t>(r) & 0b011111) << 11;
    const uint16_t G = (static_cast<uint16_t>(g) & 0b111111) << 5;
    const uint16_t B =  static_cast<uint16_t>(b) & 0b011111;
    return R | G | B;
}

static const struct {
    rgb565_t BLACK   = create_rgb565_f32(0,0,0);
    rgb565_t RED     = create_rgb565_f32(1,0,0);
    rgb565_t GREEN   = create_rgb565_f32(0,1,0);
    rgb565_t BLUE    = create_rgb565_f32(0,0,1);
    rgb565_t CYAN    = create_rgb565_f32(0,1,1);
    rgb565_t MAGENTA = create_rgb565_f32(1,0,1);
    rgb565_t YELLOW  = create_rgb565_f32(1,1,0);
    rgb565_t WHITE   = create_rgb565_f32(1,1,1);
} COLOUR;

#pragma once
#include <stdint.h>

struct Glyph {
    uint8_t width;
    uint8_t height;
    const uint8_t* data;
    uint16_t length;
};
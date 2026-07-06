#pragma once
#include <stdint.h>

#ifdef TEST_HARNESS
#define PROGMEM
#else
#include <avr/pgmspace.h>
#endif


namespace glyph {

// encodings produced by ../image_compression.py
// RLE = running length encoding
enum class Encoding: uint8_t {
    GRAYSCALE_RLE_Q4 = 0,
    BINARY_RLE_Q1 = 1,
    GRAYSCALE_Q4 = 2,
    BINARY_Q1 = 3,
    RGBA_Q256_PALETTE = 4,
};

struct Glyph {
    uint8_t width;
    uint8_t height;
    const uint8_t* data;
    uint16_t data_length;
    Encoding encoding;
};

};

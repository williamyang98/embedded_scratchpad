#pragma once
#include <stdint.h>

namespace glyph {
	
enum class Encoding: uint8_t {
    GRAYSCALE_RLE_U4 = 0,
	BINARY_RLE_U8 = 1,
};
	
struct Glyph {
    uint8_t width;
    uint8_t height;
    const uint8_t* data;
    uint16_t data_length;
	Encoding encoding;
};
	
};

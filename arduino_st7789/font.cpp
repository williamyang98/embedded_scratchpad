#include "./font.hpp"
#include <stdint.h>
#include "./tft.hpp"
#include <avr/pgmspace.h>
#include "./test/scripts/glyphs/glyph.hpp"

using namespace tft;

rgb565_t blend_rgb565_u4(rgb565_t colour_0, rgb565_t colour_1, uint8_t alpha) {
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

class BitReader {
private:
  uint8_t curr_bit = 0;
  uint16_t curr_byte = 0;
  uint8_t data_byte = 0;
  const uint8_t* data;
public:
  BitReader(const uint8_t* _data): data(_data) {}

  uint8_t read_bits(uint8_t n_bits) {
    if (curr_bit == 0) {
      data_byte = pgm_read_byte(data + curr_byte);
    }

    const uint8_t remaining_bits = 8-curr_bit;
    const uint8_t shift_bits = n_bits > remaining_bits ? remaining_bits : n_bits;
    const uint8_t mask = ~(0xFF << shift_bits);
    uint8_t bits = data_byte & mask;

    curr_bit += shift_bits;
    data_byte = data_byte >> shift_bits;
    if (curr_bit == 8) {
      curr_bit = 0;
      curr_byte += 1;
    }

    n_bits -= shift_bits;
    if (n_bits > 0) {
      const uint8_t bits_high = read_bits(n_bits);
      bits |= bits_high << shift_bits;
    }
    return bits;
  }
};

namespace gfx {

void write_digit(const Glyph& glyph, uint16_t x_start, uint16_t y_start, rgb565_t text_colour, rgb565_t background_colour) {
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
    const rgb565_t colour = blend_rgb565_u4(text_colour, background_colour, value);
    if (value == 0 || value == 3) {
      const uint8_t length = bit_reader.read_bits(8);
      for (uint16_t i = 0; i < length; i++) {
        tft::write_pixel(colour);
      }
      curr_pixel += static_cast<uint16_t>(length);
    } else {
      curr_pixel += 1;
      tft::write_pixel(colour);
    }
  }
  tft::end_write_pixel();

  return true;
}

};
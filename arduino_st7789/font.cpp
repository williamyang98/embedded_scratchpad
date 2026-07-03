#include <stdint.h>
#include <avr/pgmspace.h>
#include "./font.hpp"
#include "./tft.hpp"
#include "./bit_reader.hpp"

using namespace tft;
using namespace glyph;

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

void write_glyph_grayscale_rle_u4(const Glyph& glyph, uint16_t x_start, uint16_t y_start, rgb565_t text_colour, rgb565_t background_colour) {
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

void write_glyph_binary_rle_u8(const Glyph& glyph, uint16_t x_start, uint16_t y_start, rgb565_t text_colour, rgb565_t background_colour) {
  const uint16_t width = glyph.width;
  const uint16_t height = glyph.height;
  const uint16_t x_end = static_cast<uint16_t>(x_start+width-1);
  const uint16_t y_end = static_cast<uint16_t>(y_start+height-1);
  tft::set_write_rect(x_start, x_end, y_start, y_end);

  uint16_t curr_data = 0;
  const uint16_t total_data = glyph.data_length;

  tft::begin_write_pixel();
  constexpr uint8_t mask = 0b10000000;
  for (uint16_t curr_data = 0; curr_data < total_data; curr_data++) {
    const uint8_t data = pgm_read_byte(glyph.data + curr_data);
    const uint8_t pixel = data & mask;
    const uint8_t length = data & ~mask;
    const rgb565_t colour = pixel == 0 ? background_colour : text_colour;
    for (uint8_t i = 0; i < length; i++) {
      tft::write_pixel(colour);
    }
  }
  tft::end_write_pixel();

  return true;
}

namespace gfx {

void write_glyph(
  const glyph::Glyph& glyph,
  uint16_t x_start, uint16_t y_start,
  rgb565_t text_colour, rgb565_t background_colour
) {
  switch (glyph.encoding) {
  case glyph::Encoding::BINARY_RLE_U8: return write_glyph_binary_rle_u8(glyph, x_start, y_start, text_colour, background_colour);
  case glyph::Encoding::GRAYSCALE_RLE_U4: return write_glyph_grayscale_rle_u4(glyph, x_start, y_start, text_colour, background_colour);
  default: break;
  }
}

};
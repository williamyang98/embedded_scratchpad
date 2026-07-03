#include "./bit_reader.hpp"
#include <avr/pgmspace.h>

uint8_t BitReader::read_bits(uint8_t n_bits) {
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
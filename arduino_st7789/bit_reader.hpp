#pragma once
#include <stdint.h>

class BitReader {
private:
  uint8_t curr_bit = 0;
  uint16_t curr_byte = 0;
  uint8_t data_byte = 0;
  const uint8_t* data;
public:
  BitReader(const uint8_t* _data): data(_data) {}
  uint8_t read_bits(uint8_t n_bits);
};
#pragma once

#ifdef TEST_HARNESS
#include <stdint.h>

#define pgm_read_byte(x) *(x)
#define pgm_read_word(x) *(x)
#define pgm_read_ptr(x) *(x)
#define PSTR(x) x
#define PROGMEM

static void* memcpy_P(void* dest, const void* src, size_t n) {
    const uint8_t* src_buffer = reinterpret_cast<const uint8_t*>(src);
    uint8_t* dest_buffer = reinterpret_cast<uint8_t*>(dest);
    for (size_t i = 0; i < n; i++) {
        const uint8_t c = pgm_read_byte(src_buffer + i);
        dest_buffer[i] = c;
    }
    return dest;
}

#else
#include <avr/pgmspace.h>
#endif

template <typename T>
class FlashMemory;

#define FLASH_STRING(str) (reinterpret_cast<const FlashMemory<char>*>(PSTR(str)))

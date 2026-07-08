#pragma once

#ifdef TEST_HARNESS
#define pgm_read_byte(x) *(x)
#define pgm_read_word(x) *(x)
#define PSTR(x) x
#define PROGMEM
#else
#include <avr/pgmspace.h>
#endif

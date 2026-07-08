#pragma once

#ifdef TEST_HARNESS
#include <stdint.h>
#include <stdlib.h>
#include "./st7789.hpp"
#include "./pgmspace.h"

extern FILE* fp_in;
extern FILE* fp_out;

static struct {
    int m_baud_rate;
    void begin(int baud_rate) {
        m_baud_rate = baud_rate;
    }
    template <typename T>
    void println(T t) {
        // TODO:
    }
} Serial;

static uint32_t millis() {
    const uint64_t time_nanos = st7789->get_time_nanos();
    const uint64_t time_millis = time_nanos/static_cast<uint64_t>(1e6);
    return static_cast<uint32_t>(time_millis);
}

static void delay(unsigned long delay_ms) {
    st7789->sleep_nanos(static_cast<uint64_t>(delay_ms)*static_cast<uint64_t>(1e6));
}

static void log_frame(std::string label) {
    st7789->debug_out(fp_out, label);
}

// https://github.com/arduino/ArduinoCore-avr/blob/master/cores/arduino/WString.h#L37
class __FlashStringHelper;
#define F(string_literal) (reinterpret_cast<const __FlashStringHelper *>(PSTR(string_literal)))

#else
#define log_frame(label)
#include <Arduino.h>
#endif

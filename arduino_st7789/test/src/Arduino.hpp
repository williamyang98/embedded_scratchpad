#pragma once

#ifdef TEST_HARNESS
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <memory>
#include "./st7789.hpp"
#include "./pgmspace.h"
#include "./pipe.hpp"
#include "./tft.hpp"

#if _WIN32
#define NOMINMAX
#include <io.h>
#include <fcntl.h>
#endif

// https://github.com/arduino/ArduinoCore-avr/blob/master/cores/arduino/WString.h#L37
class __FlashStringHelper;
#define F(string_literal) (reinterpret_cast<const __FlashStringHelper *>(PSTR(string_literal)))

struct HardwareSerial {
    std::shared_ptr<Pipe> m_pipe_in = nullptr;
    FILE* m_fp_out = nullptr;
    int m_baud_rate = 0;

    void begin(int baud_rate) {
        m_pipe_in = std::make_shared<Pipe>(1024);
        m_baud_rate = baud_rate;

        #if _WIN32
        m_fp_out = stdout;
        _setmode(_fileno(m_fp_out), _O_BINARY);
        #endif
    }
    int read() {
        if (m_pipe_in == nullptr) return -1;
        return m_pipe_in->read();
    }
    size_t write(uint8_t c) {
        return fwrite(&c, sizeof(c), 1, m_fp_out);
    }
    void end() {
        if (m_pipe_in != nullptr) m_pipe_in->close();
        if (m_fp_out != nullptr) fclose(m_fp_out);
    }
    template <typename T> void println(T x) {}
    template <typename T> void print(T x) {}

};

extern HardwareSerial Serial;
extern ST7789 st7789;

static uint32_t millis() {
    const uint64_t time_nanos = st7789.get_time_nanos();
    const uint64_t time_millis = time_nanos/static_cast<uint64_t>(1e6);
    return static_cast<uint32_t>(time_millis);
}

static void delay(unsigned long delay_ms) {
    st7789.sleep_nanos(static_cast<uint64_t>(delay_ms)*static_cast<uint64_t>(1e6));
}

static void log_frame(std::string label) {
    st7789.debug_out(Serial.m_fp_out, label);
}

#else
#define log_frame(label)
#include <Arduino.h>
#endif

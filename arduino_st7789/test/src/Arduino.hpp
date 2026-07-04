#pragma once

#include <stdint.h>
#include "./tft.hpp"

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
    return 0;
}

static void delay(unsigned long delay_ms) {

}

static void log_frame(std::string label) {
    tft::st7789->debug_out(fp_out, label);
}

#pragma once

#ifdef TEST_HARNESS
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#if _WIN32
#define NOMINMAX
#include <io.h>
#include <fcntl.h>
#endif

struct HardwareSerial {
    FILE* m_fp_in = stdin;
    FILE* m_fp_out = stdout;
    int m_baud_rate = 0;

    void begin(int baud_rate) {
        m_baud_rate = baud_rate;

        #if _WIN32
        _setmode(_fileno(m_fp_in), _O_BINARY);
        _setmode(_fileno(m_fp_out), _O_BINARY);
        #endif
    }
    int read() {
        uint8_t c;
        const size_t total_read = fread(&c, sizeof(uint8_t), 1, m_fp_in);
        if (total_read == 0) return -1;
        return static_cast<uint8_t>(c);
    }
    size_t write(uint8_t c) {
        const size_t total_written = fwrite(&c, sizeof(uint8_t), 1, m_fp_out);
        fflush(m_fp_out);
        return total_written;
    }
    size_t write(const uint8_t* buffer, size_t size) {
        const size_t total_written = fwrite(buffer, sizeof(uint8_t), size, m_fp_out);
        fflush(m_fp_out);
        return total_written;
    }
    void end() {
        fclose(m_fp_in);
        fclose(m_fp_out);
    }
};

extern HardwareSerial Serial;

#else
#include <Arduino.h>
#endif

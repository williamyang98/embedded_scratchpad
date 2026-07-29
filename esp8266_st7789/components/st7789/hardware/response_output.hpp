#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef TEST_HARNESS
#include <stdio.h>
#include <stdlib.h>

#if _WIN32
#define NOMINMAX
#include <io.h>
#include <fcntl.h>
#endif
#include "../utility/cobs.hpp"

class ResponseOutput {
private:
    const FILE* m_fp_out;
    static constexpr uint8_t MAX_ENCODED_BYTES = 66;
    uint8_t m_encoded_buffer[MAX_ENCODED_BYTES] = {0};
public:
    ResponseOutput(FILE* fp_out): m_fp_out(fp_out) {
        #if _WIN32
        _setmode(_fileno(m_fp_out), _O_BINARY);
        #endif
    }
    ~ResponseOutput() {
        fclose(m_fp_out);
    }
    size_t write(const uint8_t* buffer, size_t size) {
        const size_t encoded_size = cobs::encode(buffer, size, m_encoded_buffer);
        const size_t total_written = fwrite(m_encoded_buffer, sizeof(uint8_t), encoded_size, m_fp_out);
        fflush(m_fp_out);
        return total_written;
    }
};

#else

class ResponseOutput {
public:
    ResponseOutput() {
    }
    size_t write(const uint8_t* buffer, size_t size) {
        // TODO: hook up to websocket
        return size;
    }
};

#endif

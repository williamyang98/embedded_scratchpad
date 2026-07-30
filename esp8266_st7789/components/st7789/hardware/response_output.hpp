#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef TEST_HARNESS
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#if _WIN32
#define NOMINMAX
#include <io.h>
#include <fcntl.h>
#endif
#include "../utility/cobs.hpp"

class ResponseOutput {
private:
    FILE* const m_fp_out;
    std::vector<uint8_t> m_encoded_buffer;
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
        const size_t max_encoded_size = cobs::get_maximum_encoded_size(size);
        m_encoded_buffer.resize(max_encoded_size);
        const size_t encoded_size = cobs::encode(buffer, size, m_encoded_buffer.data());
        const size_t total_written = fwrite(m_encoded_buffer.data(), sizeof(uint8_t), encoded_size, m_fp_out);
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

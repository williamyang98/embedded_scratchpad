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

#include <vector>
#include <functional>
#include <assert.h>
#include <esp_log.h>
#include <esp_err.h>

class ResponseOutput {
public:
    typedef void (*Callback)(const uint8_t* buffer, size_t size, void* args);
private:
    struct Handler {
        Callback callback;
        void* args;
        bool operator==(const Handler& other) const {
            return callback == other.callback && args == other.args;
        }
        bool operator!=(const Handler& other) const {
            return !(*this == other);
        }
    };
    std::vector<Handler> m_handlers;
public:
    ResponseOutput() {}
    size_t write(const uint8_t* buffer, size_t size) {
        // TODO: hook up to websocket
        return size;
    }
    esp_err_t attach_callback(Callback callback, void* args) {
        static const char TAG[] = "response-output-attach-callback";
        assert(callback != nullptr);
        const Handler new_handler = {
            .callback = callback,
            .args = args,
        };
        for (const auto& handler: m_handlers) {
            if (handler == new_handler) {
                ESP_LOGE(TAG, "Tried to add handler twice to a list of %u handlers", m_handlers.size());
                return ESP_FAIL;
            }
        }
        m_handlers.push_back(new_handler);
        ESP_LOGI(TAG, "Added handler for a total of %u handlers afterwards", m_handlers.size());
        return ESP_OK;
    }
    esp_err_t remove_callback(Callback callback, void* args) {
        static const char TAG[] = "response-output-remove-callback";
        assert(callback != nullptr);
        const Handler handler = {
            .callback = callback,
            .args = args,
        };
        const size_t total_handlers = m_handlers.size();
        for (size_t i = 0; i < total_handlers; i++) {
            const auto& old_handler = m_handlers[i];
            if (old_handler != handler) continue;
            // replace with last item
            m_handlers[i] = std::move(m_handlers.back());
            m_handlers.pop_back();
            ESP_LOGI(TAG, "Remove handler for a total of %u handlers remaining", m_handlers.size());
            return ESP_OK;
        }
        ESP_LOGE(TAG, "Tried to remove an untracked handler from a list of %u handlers", m_handlers.size());
        return ESP_FAIL;
    }
};

#endif

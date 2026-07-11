#pragma once
#include <stdint.h>
#include "../utility/cobs.hpp"
#include "../hardware/pgmspace.h"
#include "../hardware/serial.hpp"
#include "../graphics/rgb565.hpp"

#ifdef TEST_HARNESS
#include <format>
#include <vector>
#include <span>
#include "../hardware/st7789.hpp"
#endif


enum class ResponseHeader: uint8_t {
    ACKNOWLEDGE_COMMAND = 0x00,
    RENDER_STATUS = 0x01,
    LOG_MESSAGE = 0x02,
#ifdef TEST_HARNESS
    DEBUG_MESSAGE = 0x03,
    DEBUG_FRAME = 0x04,
#endif
};

class ResponseSender {
private:
    static constexpr uint8_t MAX_DECODED_BYTES = 64;
    static constexpr uint8_t MAX_ENCODED_BYTES = 66;
    uint8_t m_decoded_buffer[MAX_DECODED_BYTES] = {0};
    uint8_t m_encoded_buffer[MAX_ENCODED_BYTES] = {0};
public:
    void send_acknowledge(uint8_t header, bool is_success) {
        m_decoded_buffer[0] = static_cast<uint8_t>(ResponseHeader::ACKNOWLEDGE_COMMAND);
        m_decoded_buffer[1] = header;
        m_decoded_buffer[2] = is_success ? 0x01 : 0x00;
        const size_t encoded_size = cobs::encode(m_decoded_buffer, 3, m_encoded_buffer);
        Serial.write(m_encoded_buffer, encoded_size);
    }
    void send_render_status(bool is_busy) {
        m_decoded_buffer[0] = static_cast<uint8_t>(ResponseHeader::RENDER_STATUS);
        m_decoded_buffer[1] = is_busy ? 0x01 : 0x00;
        const size_t encoded_size = cobs::encode(m_decoded_buffer, 2, m_encoded_buffer);
        Serial.write(m_encoded_buffer, encoded_size);
    }
    void send_message(const FlashMemory<char>* str) {
        const uint8_t* buffer = reinterpret_cast<const uint8_t*>(str);
        // one byte for header, rest for string data
        m_decoded_buffer[0] = static_cast<uint8_t>(ResponseHeader::LOG_MESSAGE);
        uint8_t dest_i;
        size_t length = 0;
        for (dest_i = 1; dest_i < MAX_DECODED_BYTES; dest_i++) {
            const uint8_t c = pgm_read_byte(buffer + length);
            if (c == 0x00) break;
            m_decoded_buffer[dest_i] = c;
            length++;
        }
        const size_t encoded_size = cobs::encode(m_decoded_buffer, dest_i, m_encoded_buffer);
        Serial.write(m_encoded_buffer, encoded_size);
    }
    void send_message(const char* str) {
        const uint8_t* buffer = reinterpret_cast<const uint8_t*>(str);
        // one byte for header, rest for string data
        m_decoded_buffer[0] = static_cast<uint8_t>(ResponseHeader::LOG_MESSAGE);
        uint8_t dest_i;
        size_t length = 0;
        for (dest_i = 1; dest_i < MAX_DECODED_BYTES; dest_i++) {
            const uint8_t c = buffer[length];
            if (c == 0x00) break;
            m_decoded_buffer[dest_i] = c;
            length++;
        }
        const size_t encoded_size = cobs::encode(m_decoded_buffer, dest_i, m_encoded_buffer);
        Serial.write(m_encoded_buffer, encoded_size);
    }
#ifdef TEST_HARNESS
private:
    std::vector<uint8_t> m_large_decoded_buffer;
    std::vector<uint8_t> m_large_encoded_buffer;
    template <typename T>
    void push_large_value(T x) {
        constexpr size_t N = sizeof(T);
        for (size_t i = 0; i < N; i++) {
            const uint8_t b = static_cast<uint8_t>(x & 0xFF);
            m_large_decoded_buffer.push_back(b);
            x = x >> 8;
        }
    }
    template <>
    void push_large_value(uint8_t c) {
        m_large_decoded_buffer.push_back(c);
    }
    void push_large_array(std::span<const uint8_t> buf) {
        const size_t N = buf.size();
        for (size_t i = 0; i < N; i++) {
            m_large_decoded_buffer.push_back(buf[i]);
        }
    }
public:
    template <class... T>
    void debug_message(const std::format_string<T...> fmt, T&&... args) {
        const std::string message = std::format(fmt, args...);
        m_large_decoded_buffer.resize(0);
        // one byte for header, rest for string data
        push_large_value(static_cast<uint8_t>(ResponseHeader::DEBUG_MESSAGE));
        push_large_array(std::span(
            reinterpret_cast<const uint8_t*>(message.data()),
            message.length()
        ));

        const size_t decoded_size = m_large_decoded_buffer.size();
        const size_t max_encoded_size = cobs::get_maximum_encoded_size(decoded_size);
        m_large_encoded_buffer.resize(max_encoded_size);
        const size_t encoded_size = cobs::encode(m_large_decoded_buffer.data(), decoded_size, m_large_encoded_buffer.data());
        Serial.write(m_large_encoded_buffer.data(), encoded_size);
    }
    template <class... T>
    void debug_frame(const std::format_string<T...> fmt, T&&... args) {
        const std::string label = std::format(fmt, args...);

        m_large_decoded_buffer.resize(0);
        push_large_value(static_cast<uint8_t>(ResponseHeader::DEBUG_FRAME));
        push_large_value(g_st7789.m_rect.x_start);
        push_large_value(g_st7789.m_rect.x_end);
        push_large_value(g_st7789.m_rect.y_start);
        push_large_value(g_st7789.m_rect.y_end);
        push_large_value(g_st7789.m_cursor.x);
        push_large_value(g_st7789.m_cursor.y);
        push_large_value(g_st7789.m_width);
        push_large_value(g_st7789.m_height);
        push_large_value(g_st7789.m_brightness);
        push_large_value(g_st7789.m_is_hardware_reset);
        g_st7789.m_is_hardware_reset = 0;

        push_large_value(static_cast<uint32_t>(label.length()));
        push_large_array(std::span(
            reinterpret_cast<const uint8_t*>(label.data()),
            label.length()
        ));

        const size_t total_pixels = g_st7789.m_buffer.size();
        const size_t total_bytes = total_pixels*sizeof(rgb565_t);
        push_large_array(std::span(
            reinterpret_cast<const uint8_t*>(g_st7789.m_buffer.data()),
            total_bytes
        ));

        const size_t decoded_size = m_large_decoded_buffer.size();
        const size_t max_encoded_size = cobs::get_maximum_encoded_size(decoded_size);
        m_large_encoded_buffer.resize(max_encoded_size);
        const size_t encoded_size = cobs::encode(m_large_decoded_buffer.data(), decoded_size, m_large_encoded_buffer.data());
        Serial.write(m_large_encoded_buffer.data(), encoded_size);
    }
#endif
};

extern ResponseSender g_response_sender;

#ifdef TEST_HARNESS
#define DEBUG_MESSAGE(...) g_response_sender.debug_message(__VA_ARGS__)
#define DEBUG_FRAME(...) g_response_sender.debug_frame(__VA_ARGS__)
#else
#define DEBUG_MESSAGE(...)
#define DEBUG_FRAME(...)
#endif

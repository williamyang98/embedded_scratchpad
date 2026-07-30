#pragma once
#include <stdint.h>
#include "./app/commands.hpp"
#include "./utility/cobs.hpp"

class CobsDecoder {
private:
    CommandParser& m_parser;
    static constexpr uint8_t MAX_ENCODED_BYTES = 32;
    static constexpr uint8_t MAX_DECODED_BYTES = 30;
    uint8_t m_encoded_circular_buffer[MAX_ENCODED_BYTES] = {0};
    uint8_t m_temp_buffer[MAX_ENCODED_BYTES] = {0};
    uint8_t m_decoded_buffer[MAX_DECODED_BYTES] = {0};
    uint8_t m_incoming_write_index = 0;
    uint8_t m_incoming_length = 0;
    uint8_t m_decoded_length = 0;
public:
    CobsDecoder(CommandParser& parser): m_parser(parser) {}
    void handle_incoming_byte(const uint8_t c) {
        if (!read_incoming_byte(c)) return;
        if (m_decoded_length == 0) return;
        m_parser.parse_command(m_decoded_buffer, m_decoded_length);
    }
private:
    bool read_incoming_byte(const uint8_t c) {
        m_encoded_circular_buffer[m_incoming_write_index] = c;
        m_incoming_write_index++;
        if (m_incoming_write_index >= MAX_ENCODED_BYTES) m_incoming_write_index = 0;
        if (m_incoming_length < MAX_ENCODED_BYTES) m_incoming_length++; // write over existing data
        if (c != cobs::DELIMITER_BYTE) return false;
        read_cobs_frame();
        return true;
    }
    void read_cobs_frame() {
        // unwrap circular buffer
        int16_t _read_index = static_cast<int16_t>(m_incoming_write_index) - static_cast<int16_t>(m_incoming_length);
        if (_read_index < 0) {
            _read_index += static_cast<int16_t>(MAX_ENCODED_BYTES);
        }
        uint8_t read_index = static_cast<uint8_t>(_read_index);
        for (uint8_t i = 0; i < m_incoming_length; i++) {
            m_temp_buffer[i] = m_encoded_circular_buffer[read_index];
            read_index++;
            if (read_index >= MAX_ENCODED_BYTES) read_index = 0;
        }
        const size_t decoded_length = cobs::decode(m_temp_buffer, static_cast<size_t>(m_incoming_length), m_decoded_buffer);
        m_decoded_length = static_cast<uint8_t>(decoded_length);
        m_incoming_write_index = 0;
        m_incoming_length = 0;
    }
};

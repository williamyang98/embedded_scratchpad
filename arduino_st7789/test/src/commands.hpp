#pragma once
#include <stdint.h>
#include "./app.hpp"
#include "./cobs.hpp"
#include "./response.hpp"

enum class CommandHeader: uint8_t {
    TRIGGER_RENDER = 0x00,
    SET_TEMPERATURE = 0x01,
    SET_HUMIDITY = 0x02,
    SET_TIME_24_HOUR = 0x03,
    SET_WIND_KPH = 0x04,
};

class CobsDecoder {
private:
    static constexpr uint8_t MAX_ENCODED_BYTES = 32;
    static constexpr uint8_t MAX_DECODED_BYTES = 30;
    uint8_t m_encoded_circular_buffer[MAX_ENCODED_BYTES] = {0};
    uint8_t m_temp_buffer[MAX_ENCODED_BYTES] = {0};
    uint8_t m_decoded_buffer[MAX_DECODED_BYTES] = {0};
    uint8_t m_incoming_write_index = 0;
    uint8_t m_incoming_length = 0;
    uint8_t m_decoded_length = 0;
public:
    const uint8_t* get_decoded_buffer() const { return m_decoded_buffer; }
    const uint8_t get_decoded_length() const { return m_decoded_length; }
    bool read_incoming_byte(const uint8_t c) {
        m_encoded_circular_buffer[m_incoming_write_index] = c;
        m_incoming_write_index++;
        if (m_incoming_write_index >= MAX_ENCODED_BYTES) m_incoming_write_index = 0;
        if (m_incoming_length < MAX_ENCODED_BYTES) m_incoming_length++; // write over existing data
        if (c != cobs::DELIMITER_BYTE) return false;
        read_cobs_frame();
        return true;
    }
private:
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

class CommandParser {
private:
    CobsDecoder cobs_decoder;
public:
    void read_incoming_byte(const uint8_t c) {
        if (!cobs_decoder.read_incoming_byte(c)) return;
        const uint8_t* buffer = cobs_decoder.get_decoded_buffer();
        const size_t length = cobs_decoder.get_decoded_length();
        if (length == 0) return;
        const bool success = parse_command(buffer, length);
        response_sender.send_acknowledge(buffer[0], success);
    }
private:
    bool parse_command(const uint8_t* buffer, const size_t length) {
        const auto header = static_cast<CommandHeader>(buffer[0]);
        if (header == CommandHeader::TRIGGER_RENDER) {
            app.render_all();
            return true;
        }
        if (header == CommandHeader::SET_TEMPERATURE) {
            if (length != 3) return false;
            int16_t temperature = 0;
            temperature |= static_cast<int16_t>(buffer[1]) << 8;
            temperature |= static_cast<int16_t>(buffer[2]);
            app.set_temperature(temperature);
            return true;
        }
        if (header == CommandHeader::SET_HUMIDITY) {
            if (length != 3) return false;
            uint16_t humidity = 0;
            humidity |= static_cast<uint16_t>(buffer[1]) << 8;
            humidity |= static_cast<uint16_t>(buffer[2]);
            app.set_humidity(humidity);
            return true;
        }
        if (header == CommandHeader::SET_WIND_KPH) {
            if (length != 3) return false;
            uint16_t wind_kph = 0;
            wind_kph |= static_cast<uint16_t>(buffer[1]) << 8;
            wind_kph |= static_cast<uint16_t>(buffer[2]);
            app.set_wind(wind_kph);
            return true;
        }
        if (header == CommandHeader::SET_TIME_24_HOUR) {
            if (length != 5) return false;
            uint16_t time_24_hour = 0;
            time_24_hour |= static_cast<uint16_t>(buffer[1]) << 8;
            time_24_hour |= static_cast<uint16_t>(buffer[2]);
            const bool is_show_24_hour = buffer[3] != 0;
            const bool is_show_leading_zero = buffer[4] != 0;
            app.set_time(time_24_hour);
            app.set_time_show_24_hour(is_show_24_hour);
            app.set_time_show_leading_zero(is_show_leading_zero);
            return true;
        }
        return false;
    }
};

extern CommandParser command_parser;

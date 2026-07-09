#pragma once
#include <stdint.h>
#include "./app.hpp"

enum class CommandHeader: uint8_t {
    TRIGGER_RENDER = 0x00,
    SET_TEMPERATURE = 0x01,
    SET_HUMIDITY = 0x02,
    SET_TIME_24_HOUR = 0x03,
    SET_WIND_KPH = 0x04,
};

class CommandParser {
private:
    App& app;
    static constexpr uint8_t MAX_ENCODED_BYTES = 32;
    static constexpr uint8_t MAX_DECODED_BYTES = 30;
    uint8_t m_encoded_circular_buffer[MAX_ENCODED_BYTES] = {0};
    uint8_t m_temp_buffer[MAX_ENCODED_BYTES] = {0};
    uint8_t m_decoded_buffer[MAX_DECODED_BYTES] = {0};

    uint8_t m_incoming_write_index = 0;
    uint8_t m_incoming_length = 0;
public:
    CommandParser(App& _app): app(_app) {}
    void read_incoming_byte(const uint8_t c) {
        m_encoded_circular_buffer[m_incoming_write_index] = c;
        m_incoming_write_index++;
        if (m_incoming_write_index >= MAX_ENCODED_BYTES) m_incoming_write_index = 0;
        if (m_incoming_length < MAX_ENCODED_BYTES) m_incoming_length++; // write over existing data
        if (c == COBS_DELIMITER_BYTE) read_cobs_frame();
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
        const uint8_t decoded_length = cobs_decode(m_temp_buffer, m_incoming_length, m_decoded_buffer);
        m_incoming_write_index = 0;
        m_incoming_length = 0;
        parse_command(m_decoded_buffer, decoded_length);
    }

    void parse_command(const uint8_t* buffer, const uint8_t length) {
        if (length == 0) return;
        const auto header = static_cast<CommandHeader>(buffer[0]);
        if (header == CommandHeader::TRIGGER_RENDER) {
            app.render_all();
            return;
        }
        if (header == CommandHeader::SET_TEMPERATURE) {
            if (length != 3) return;
            int16_t temperature = 0;
            temperature |= static_cast<int16_t>(buffer[1]) << 8;
            temperature |= static_cast<int16_t>(buffer[2]);
            app.set_temperature(temperature);
            return;
        }
        if (header == CommandHeader::SET_HUMIDITY) {
            if (length != 3) return;
            uint16_t humidity = 0;
            humidity |= static_cast<uint16_t>(buffer[1]) << 8;
            humidity |= static_cast<uint16_t>(buffer[2]);
            app.set_humidity(humidity);
            return;
        }
        if (header == CommandHeader::SET_WIND_KPH) {
            if (length != 3) return;
            uint16_t wind_kph = 0;
            wind_kph |= static_cast<uint16_t>(buffer[1]) << 8;
            wind_kph |= static_cast<uint16_t>(buffer[2]);
            app.set_wind(wind_kph);
            return;
        }
        if (header == CommandHeader::SET_TIME_24_HOUR) {
            if (length != 5) return;
            uint16_t time_24_hour = 0;
            time_24_hour |= static_cast<uint16_t>(buffer[1]) << 8;
            time_24_hour |= static_cast<uint16_t>(buffer[2]);
            const bool is_show_24_hour = buffer[3] != 0;
            const bool is_show_leading_zero = buffer[4] != 0;
            app.set_time(time_24_hour);
            app.set_time_show_24_hour(is_show_24_hour);
            app.set_time_show_leading_zero(is_show_leading_zero);
            return;
        }
    }
};

#ifdef TEST_HARNESS
#include "./pipe.hpp"
#include <array>

// sender counterpart to CommandParser::parse_command
class CommandSender {
private:
    std::shared_ptr<Pipe> m_pipe_in = nullptr;
public:
    CommandSender(std::shared_ptr<Pipe> pipe_in): m_pipe_in(pipe_in) {}

    void trigger_render() {
        std::array<const uint8_t, 1> packet = {
            static_cast<uint8_t>(CommandHeader::TRIGGER_RENDER),
        };
        send_packet(packet);
    }

    void set_temperature(int16_t temperature) {
        std::array<const uint8_t, 3> packet = {
            static_cast<uint8_t>(CommandHeader::SET_TEMPERATURE),
            static_cast<uint8_t>(temperature >> 8),
            static_cast<uint8_t>(temperature & 0xFF),
        };
        send_packet(packet);
    }

    void set_humidity(uint16_t humidity) {
        std::array<const uint8_t, 3> packet = {
            static_cast<uint8_t>(CommandHeader::SET_HUMIDITY),
            static_cast<uint8_t>(humidity >> 8),
            static_cast<uint8_t>(humidity & 0xFF),
        };
        send_packet(packet);
    }

    void set_time_24_hour(uint16_t time_24_hour, bool is_show_24_hour, bool is_show_leading_zero) {
        std::array<const uint8_t, 5> packet = {
            static_cast<uint8_t>(CommandHeader::SET_TIME_24_HOUR),
            static_cast<uint8_t>(time_24_hour >> 8),
            static_cast<uint8_t>(time_24_hour & 0xFF),
            static_cast<uint8_t>(is_show_24_hour ? 0x01 : 0x00),
            static_cast<uint8_t>(is_show_leading_zero ? 0x01 : 0x00),
        };
        send_packet(packet);
    }

    void set_wind_kph(uint16_t wind_kph) {
        std::array<const uint8_t, 3> packet = {
            static_cast<uint8_t>(CommandHeader::SET_WIND_KPH),
            static_cast<uint8_t>(wind_kph >> 8),
            static_cast<uint8_t>(wind_kph & 0xFF),
        };
        send_packet(packet);
    }

    void close() {
        m_pipe_in->close();
    }
private:
    template <size_t N>
    void send_packet(std::array<const uint8_t, N> data_in) {
        std::array<uint8_t, N+2> data_out;
        cobs_encode(data_in.data(), uint8_t(data_in.size()), data_out.data());
        m_pipe_in->write_block(data_out);
    }
};

#endif

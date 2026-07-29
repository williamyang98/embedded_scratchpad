#pragma once
#include <stdint.h>
#include "./app.hpp"
#include "../hardware/tft.hpp"
#include "../utility/cobs.hpp"
#include "./response.hpp"

enum class CommandHeader: uint8_t {
    TRIGGER_RENDER = 0x00,
    // weather page
    SET_TEMPERATURE = 0x01,
    SET_HUMIDITY = 0x02,
    SET_TIME_24_HOUR = 0x03,
    SET_WIND_KPH = 0x04,
    SET_WEATHER_ICON = 0x05,
    SET_LOCATION = 0x06,
    SET_WEATHER_DESCRIPTION = 0x07,
    SET_MOON_PHASE = 0x08,
    // set page
    SET_SCREEN_BRIGHTNESS = 0xFE,
    SET_PAGE = 0xFF,
};

class CommandParser {
private:
    ResponseSender& m_response_sender;
    App& m_app;
public:
    CommandParser(ResponseSender& response_sender, App& app)
    :   m_response_sender(response_sender),
        m_app(app)
    {}
    void parse_command(const uint8_t* buffer, const size_t length) {
        const bool success = _parse_command(buffer, length);
        m_response_sender.send_acknowledge(buffer[0], success);
    }
private:
    bool _parse_command(const uint8_t* buffer, const size_t length) {
        const auto header = static_cast<CommandHeader>(buffer[0]);
        WeatherPage& weather_page = m_app.get_weather_page();
        if (header == CommandHeader::TRIGGER_RENDER) {
            m_app.render_all();
            return true;
        }
        if (header == CommandHeader::SET_PAGE) {
            if (length != 2) return false;
            const uint8_t page_index = buffer[1];
            if (page_index >= TOTAL_APP_PAGES) return false;
            const AppPage page = static_cast<AppPage>(buffer[1]);
            m_app.set_page(page);
            return true;
        }
        if (header == CommandHeader::SET_SCREEN_BRIGHTNESS) {
            if (length != 2) return false;
            const uint8_t brightness = buffer[1];
            tft::set_brightness(brightness);
            return true;
        }
        if (header == CommandHeader::SET_TEMPERATURE) {
            if (length != 3) return false;
            int16_t temperature = 0;
            temperature |= static_cast<int16_t>(buffer[1]) << 8;
            temperature |= static_cast<int16_t>(buffer[2]);
            weather_page.set_temperature(temperature);
            return true;
        }
        if (header == CommandHeader::SET_HUMIDITY) {
            if (length != 3) return false;
            uint16_t humidity = 0;
            humidity |= static_cast<uint16_t>(buffer[1]) << 8;
            humidity |= static_cast<uint16_t>(buffer[2]);
            weather_page.set_humidity(humidity);
            return true;
        }
        if (header == CommandHeader::SET_WIND_KPH) {
            if (length != 3) return false;
            uint16_t wind_kph = 0;
            wind_kph |= static_cast<uint16_t>(buffer[1]) << 8;
            wind_kph |= static_cast<uint16_t>(buffer[2]);
            weather_page.set_wind(wind_kph);
            return true;
        }
        if (header == CommandHeader::SET_TIME_24_HOUR) {
            if (length != 5) return false;
            uint16_t time_24_hour = 0;
            time_24_hour |= static_cast<uint16_t>(buffer[1]) << 8;
            time_24_hour |= static_cast<uint16_t>(buffer[2]);
            const bool is_show_24_hour = buffer[3] != 0;
            const bool is_show_leading_zero = buffer[4] != 0;
            weather_page.set_time(time_24_hour);
            weather_page.set_time_show_24_hour(is_show_24_hour);
            weather_page.set_time_show_leading_zero(is_show_leading_zero);
            return true;
        }
        if (header == CommandHeader::SET_WEATHER_ICON) {
            if (length != 2) return false;
            const WeatherIcon icon = static_cast<WeatherIcon>(buffer[1]);
            if (get_weather_icon(icon) == nullptr) return false;
            weather_page.set_weather_icon(icon);
            return true;
        }
        if (header == CommandHeader::SET_LOCATION) {
            const size_t string_length = length-1;
            const char* location = reinterpret_cast<const char*>(&buffer[1]);
            weather_page.set_location(location, string_length);
            return true;
        }
        if (header == CommandHeader::SET_WEATHER_DESCRIPTION) {
            const size_t string_length = length-1;
            const char* weather_description = reinterpret_cast<const char*>(&buffer[1]);
            weather_page.set_weather_description(weather_description, string_length);
            return true;
        }
        if (header == CommandHeader::SET_MOON_PHASE) {
            if (length != 2) return false;
            const MoonPhase phase = static_cast<MoonPhase>(buffer[1]);
            if (get_moon_phase_icon(phase) == nullptr) return false;
            weather_page.set_moon_phase(phase);
            return true;
        }
        return false;
    }
};

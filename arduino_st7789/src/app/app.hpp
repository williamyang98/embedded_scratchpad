#pragma once

#include "../hardware/tft.hpp"
#include "../hardware/pgmspace.h"
#include "../utility/printers.hpp"
#include "../graphics/rgb565.hpp"
#include "../graphics/colour_functions.hpp"
#include "../graphics/render_glyphs.hpp"
#include "../glyphs/large_font.hpp"
#include "../glyphs/small_font.hpp"
#include "../glyphs/icons.hpp"
#include "./moon_phases.hpp"
#include "./wind_speeds.hpp"
#include "./weather_icons.hpp"
#include "./response.hpp"

class App {
public:
    static constexpr uint8_t MAX_TEXT_LENGTH = 32;
private:
    uint16_t m_time_24_hour = 0;
    WeatherIcon m_weather_icon = WeatherIcon::SUNNY;
    int16_t m_temperature_celcius = 0;
    uint16_t m_humidity_percent = 0;
    char m_location[MAX_TEXT_LENGTH+1] = {0};
    char m_weather_description[MAX_TEXT_LENGTH+1] = {0};
    uint16_t m_wind_kph = 0;
    MoonPhase m_moon_phase = MoonPhase::FULL_MOON;
    struct {
        bool is_show_24_hour_time = false;
        bool is_show_leading_zero = false;
    } m_time_display_settings;
    struct {
        RightToLeftPrinter time;
        LeftToRightPrinter weather_icon;
        RightToLeftPrinter temperature;
        RightToLeftPrinter humidity;
        LeftToRightPrinter location;
        LeftToRightPrinter weather_description;
        LeftToRightPrinter wind;
        LeftToRightPrinter moon;
        void reset() {
            time.reset_x_end();
            weather_icon.reset_x_start();
            temperature.reset_x_end();
            humidity.reset_x_end();
            location.reset_x_start();
            weather_description.reset_x_start();
            wind.reset_x_start();
            moon.reset_x_start();
        }
        void set_text_colour(rgb565_t text_colour) {
            time.text_colour = text_colour;
            weather_icon.text_colour = text_colour;
            temperature.text_colour = text_colour;
            humidity.text_colour = text_colour;
            location.text_colour = text_colour;
            weather_description.text_colour = text_colour;
            wind.text_colour = text_colour;
            moon.text_colour = text_colour;
        }
    } m_printers;
    rgb565_t m_text_colour = COLOUR.WHITE;
    uint16_t m_x_margin = 10;
    uint16_t m_y_margin = 10;
    struct {
        RadialBackgroundColour freezing;
        RadialBackgroundColour cold;
        RadialBackgroundColour normal;
        RadialBackgroundColour warm;
        RadialBackgroundColour hot;
    } m_background_colour;
    enum class BackgroundState: uint8_t {
        FREEZING = 0,
        COLD = 1,
        NORMAL = 2,
        WARM = 3,
        HOT = 4,
    };
    BackgroundState m_background_state = BackgroundState::FREEZING;
    struct {
        bool background;
        bool time;
        bool weather_icon;
        bool temperature;
        bool humidity;
        bool location;
        bool weather_description;
        bool wind;
        bool moon;
        void set_all(bool state) {
            background = state;
            time = state;
            weather_icon = state;
            temperature = state;
            humidity = state;
            location = state;
            weather_description = state;
            wind = state;
            moon = state;
        }
    } m_render_mask;
public:
    App() {
        m_render_mask.set_all(true);
        // create backgrounds
        m_background_colour.freezing.background_colour = create_rgb565_u8(40, 40, 80);
        m_background_colour.freezing.delta_colour = create_rgb565_bits(2,4,2);
        m_background_colour.cold.background_colour = create_rgb565_u8(30, 30, 80);
        m_background_colour.cold.delta_colour = create_rgb565_bits(1,1,2);
        m_background_colour.normal.background_colour = create_rgb565_u8(30, 80, 30);
        m_background_colour.normal.delta_colour = create_rgb565_bits(1,4,1);
        m_background_colour.warm.background_colour = create_rgb565_u8(80, 50, 20);
        m_background_colour.warm.delta_colour = create_rgb565_bits(2,2,1);
        m_background_colour.hot.background_colour = create_rgb565_u8(80, 20, 20);
        m_background_colour.hot.delta_colour = create_rgb565_bits(2,1,1);
        // set line heights
        {
            uint16_t y_text_end = 0;
            // large text
            y_text_end += m_y_margin+large_font::MAX_HEIGHT;
            m_printers.time.y_end = y_text_end-1;
            y_text_end += m_y_margin+large_font::MAX_HEIGHT;
            m_printers.temperature.y_end = y_text_end-1;
            y_text_end += m_y_margin+large_font::MAX_HEIGHT;
            m_printers.humidity.y_end = y_text_end-1;
            m_printers.weather_icon.y_end = (m_printers.temperature.y_end+m_printers.humidity.y_end)/2;
            // small text
            y_text_end += 2*m_y_margin+small_font::MAX_HEIGHT;
            m_printers.location.y_end = y_text_end-1;
            y_text_end += m_y_margin+small_font::MAX_HEIGHT;
            m_printers.weather_description.y_end = y_text_end-1;
            y_text_end += m_y_margin+small_font::MAX_HEIGHT;
            m_printers.wind.y_end = y_text_end-1;
            y_text_end += m_y_margin+small_font::MAX_HEIGHT;
            m_printers.moon.y_end = y_text_end-1;
        }
        m_printers.set_text_colour(m_text_colour);
    }

    void render_all() {
        g_response_sender.send_render_status(true);
        if (m_render_mask.background) {
            render_background();
            m_printers.reset();
            m_render_mask.set_all(true);
        }
        if (m_render_mask.time) render_time();
        if (m_render_mask.temperature) render_temperature();
        if (m_render_mask.weather_icon) render_weather_icon();
        if (m_render_mask.humidity) render_humidity();
        if (m_render_mask.location) render_location();
        if (m_render_mask.weather_description) render_weather_description();
        if (m_render_mask.wind) render_wind();
        if (m_render_mask.moon) render_moon();
        m_render_mask.set_all(false);
        g_response_sender.send_render_status(false);
        DEBUG_FRAME("render_all: temperature={0}°C, humidity={1}%, wind={2}kph", m_temperature_celcius, m_humidity_percent, m_wind_kph);
    }

    void set_time(uint16_t time_24_hour) {
        time_24_hour = time_24_hour % 2400;
        const bool is_changed = m_time_24_hour != time_24_hour;
        m_render_mask.time |= is_changed;
        m_time_24_hour = time_24_hour;
    }

    void set_time_show_24_hour(bool is_show_24_hour) {
        const bool is_changed = m_time_display_settings.is_show_24_hour_time != is_show_24_hour;
        m_render_mask.time |= is_changed;
        m_time_display_settings.is_show_24_hour_time = is_show_24_hour;
    }

    void set_time_show_leading_zero(bool is_show_leading_zero) {
        const bool is_changed = m_time_display_settings.is_show_leading_zero != is_show_leading_zero;
        m_render_mask.time |= is_changed;
        m_time_display_settings.is_show_leading_zero = is_show_leading_zero;
    }

    bool set_weather_icon(WeatherIcon icon) {
        const bool is_changed = m_weather_icon != icon;
        m_render_mask.weather_icon |= is_changed;
        m_weather_icon = icon;
        return is_changed;
    }

    void set_temperature(int16_t temperature_celcius) {
        const bool is_changed = m_temperature_celcius != temperature_celcius;
        m_render_mask.temperature |= is_changed;
        m_render_mask.weather_description |= is_changed;
        m_temperature_celcius = temperature_celcius;
        update_background();
    }

    void set_humidity(uint16_t humidity_percent) {
        const bool is_changed = m_humidity_percent != humidity_percent;
        m_render_mask.humidity |= is_changed;
        m_humidity_percent = humidity_percent;
    }

    void set_location(const char* location, size_t length) {
        size_t i = 0;
        while (true) {
            if (i >= MAX_TEXT_LENGTH) break;
            if (i >= length) break;
            m_location[i] = location[i];
            i++;
        }
        m_location[i] = 0;
        m_render_mask.location = true;
    }

    void set_weather_description(const char* location, size_t length) {
        size_t i = 0;
        while (true) {
            if (i >= MAX_TEXT_LENGTH) break;
            if (i >= length) break;
            m_weather_description[i] = location[i];
            i++;
        }
        m_weather_description[i] = 0;
        m_render_mask.weather_description = true;
    }

    void set_wind(uint16_t wind_kph) {
        const bool is_changed = m_wind_kph != wind_kph;
        m_render_mask.wind |= is_changed;
        m_wind_kph = wind_kph;
    }

    bool set_moon_phase(MoonPhase phase) {
        const bool is_changed = phase != m_moon_phase;
        m_render_mask.moon |= is_changed;
        m_moon_phase = phase;
        return is_changed;
    }
private:
    RadialBackgroundColour& get_background_colour() {
        switch (m_background_state) {
        case BackgroundState::FREEZING: return m_background_colour.freezing;
        case BackgroundState::COLD: return m_background_colour.cold;
        case BackgroundState::NORMAL: return m_background_colour.normal;
        case BackgroundState::WARM: return m_background_colour.warm;
        case BackgroundState::HOT: return m_background_colour.hot;
        default: return m_background_colour.freezing;
        }
    }

    bool update_background() {
        BackgroundState new_state = m_background_state;
        if (m_temperature_celcius < 0) {
            new_state = BackgroundState::FREEZING;
        } else if (m_temperature_celcius < 200) {
            new_state = BackgroundState::COLD;
        } else if (m_temperature_celcius < 250) {
            new_state = BackgroundState::NORMAL;
        } else if (m_temperature_celcius < 300) {
            new_state = BackgroundState::WARM;
        } else {
            new_state = BackgroundState::HOT;
        }
        const bool is_changed = new_state != m_background_state;
        m_render_mask.background |= is_changed;
        m_background_state = new_state;
        return false;
    }

    void render_background() {
        auto& background_colour = get_background_colour();
        background_colour.x_start = 0;
        background_colour.y_start = 0;
        background_colour.x_end = tft::SCREEN_WIDTH-1;
        background_colour.y_end = tft::SCREEN_HEIGHT-1;
        background_colour.fill();
    }

    void render_time() {
        auto& background_colour = get_background_colour();
        namespace font = large_font;
        const auto get_glyph = &font::get_glyph;
        auto& printer = m_printers.time;
        printer.x_end = tft::SCREEN_WIDTH-1-m_x_margin;
        uint16_t time = m_time_24_hour;
        if (!m_time_display_settings.is_show_24_hour_time) {
            if (time >= 2400) time -= 2400;
            const bool is_pm = time >= 1200;
            printer.print_string(is_pm ? FLASH_STRING("PM") : FLASH_STRING("AM"), background_colour, get_glyph);
            if (time >= 1300) time -= 1200; // 13:00 -> 1:00PM
            else if (time < 100) time += 1200; // 00:00 -> 12:00AM
        }
        const auto digits = Digits(time);
        printer.print_char('0'+digits[0], background_colour, get_glyph);
        printer.print_char('0'+digits[1], background_colour, get_glyph);
        printer.print_char(':', background_colour, get_glyph);
        printer.print_char('0'+digits[2], background_colour, get_glyph);
        if (m_time_display_settings.is_show_leading_zero || digits[3] > 0) {
            printer.print_char('0'+digits[3], background_colour, get_glyph);
        }
        printer.cleanup_previous_prints(font::MAX_HEIGHT, background_colour);
    }

    void render_weather_icon() {
        auto& background_colour = get_background_colour();
        auto& printer = m_printers.weather_icon;
        printer.x_start = m_x_margin;
        const auto* icon = get_weather_icon(m_weather_icon);
        printer.print_glyph(icon, background_colour);
        printer.cleanup_previous_prints(WEATHER_ICON_MAX_HEIGHT, background_colour);
    }

    void render_temperature() {
        auto& background_colour = get_background_colour();
        namespace font = large_font;
        const auto get_glyph = &font::get_glyph;
        auto& printer = m_printers.temperature;
        printer.x_end = tft::SCREEN_WIDTH-1-m_x_margin;
        const auto digits = Digits(m_temperature_celcius);
        printer.print_char('C', background_colour, get_glyph);
        printer.print_char(0xB0, background_colour, get_glyph);
        printer.print_char('0'+digits[0], background_colour, get_glyph);
        printer.print_char('.', background_colour, get_glyph);
        printer.print_char('0'+digits[1], background_colour, get_glyph);
        if (digits.leading_non_zero_digit_index >= 2) printer.print_char('0'+digits[2], background_colour, get_glyph);
        if (digits.is_minus) printer.print_char('-', background_colour, get_glyph);
        printer.cleanup_previous_prints(font::MAX_HEIGHT, background_colour);
    }

    void render_humidity() {
        auto& background_colour = get_background_colour();
        namespace font = large_font;
        const auto get_glyph = &font::get_glyph;
        auto& printer = m_printers.humidity;
        printer.x_end = tft::SCREEN_WIDTH-1-m_x_margin;
        const auto digits = Digits(m_humidity_percent);
        printer.print_char('%', background_colour, get_glyph);
        printer.print_char('0'+digits[0], background_colour, get_glyph);
        printer.print_char('.', background_colour, get_glyph);
        printer.print_char('0'+digits[1], background_colour, get_glyph);
        if (digits.leading_non_zero_digit_index >= 2) printer.print_char('0'+digits[2], background_colour, get_glyph);
        if (digits.leading_non_zero_digit_index >= 3) printer.print_char('0'+digits[3], background_colour, get_glyph);
        printer.cleanup_previous_prints(font::MAX_HEIGHT, background_colour);
    }

    void render_location() {
        auto& background_colour = get_background_colour();
        namespace font = small_font;
        const auto get_glyph = &font::get_glyph;
        auto& printer = m_printers.location;
        printer.x_start = m_x_margin;
        const auto* icon = icons::small::get_icon(icons::small::Icon::LOCATION_PIN);
        if (icon != nullptr) {
            printer.print_glyph(icon, background_colour);
            printer.print_char(' ', background_colour, get_glyph);
        }
        const char* description = m_location;
        printer.print_string(description, background_colour, get_glyph);
        printer.cleanup_previous_prints(font::MAX_HEIGHT, background_colour);
    }

    void render_weather_description() {
        auto& background_colour = get_background_colour();
        namespace font = small_font;
        const auto get_glyph = &font::get_glyph;
        auto& printer = m_printers.weather_description;
        printer.x_start = m_x_margin;
        const auto* icon = icons::small::get_icon(icons::small::Icon::WINDSOCK);
        if (icon != nullptr) {
            printer.print_glyph(icon, background_colour);
            printer.print_char(' ', background_colour, get_glyph);
        }
        const char* description = m_weather_description;
        printer.print_string(description, background_colour, get_glyph);
        printer.cleanup_previous_prints(font::MAX_HEIGHT, background_colour);
    }

    void render_wind() {
        auto& background_colour = get_background_colour();
        namespace font = small_font;
        const auto get_glyph = &font::get_glyph;
        auto& printer = m_printers.wind;
        printer.x_start = m_x_margin;
        const auto* icon = icons::small::get_icon(icons::small::Icon::WIND);
        if (icon != nullptr) {
            printer.print_glyph(icon, background_colour);
            printer.print_char(' ', background_colour, get_glyph);
        }
        const auto digits = Digits(m_wind_kph);
        int8_t leading_non_zero_digit_index = digits.leading_non_zero_digit_index;
        if (leading_non_zero_digit_index < 1) leading_non_zero_digit_index = 1;
        for (int8_t i = leading_non_zero_digit_index; i >= 0; i--) {
            const uint8_t digit = digits[i];
            if (i == 0) printer.print_char('.', background_colour, get_glyph);
            printer.print_char('0'+digit, background_colour, get_glyph);
        }
        printer.print_string(FLASH_STRING("KPH"), background_colour, get_glyph);
        const WindCategory category = get_wind_speed_category(m_wind_kph);
        const auto* wind_description = get_wind_speed_description(category);
        if (wind_description != nullptr) {
            printer.print_char(' ', background_colour, get_glyph);
            printer.print_string(wind_description, background_colour, get_glyph);
        }
        printer.cleanup_previous_prints(font::MAX_HEIGHT, background_colour);
    }

    void render_moon() {
        auto& background_colour = get_background_colour();
        namespace font = small_font;
        const auto get_glyph = &font::get_glyph;
        auto& printer = m_printers.moon;
        printer.x_start = m_x_margin;
        const MoonPhaseIcon* icon = get_moon_phase_icon(m_moon_phase);
        const FlashMemory<char>* description = get_moon_phase_description(m_moon_phase);
        if (icon != nullptr) {
            const auto* glyph = icon->get_glyph();
            const bool is_x_mirrored = icon->is_horizontally_flipped;
            g_glyph_rgba_q256_palette_render_settings.x_mirror = is_x_mirrored;
            printer.print_glyph(glyph, background_colour);
            g_glyph_rgba_q256_palette_render_settings.x_mirror = false;
            printer.print_char(' ', background_colour, get_glyph);
        }
        printer.print_string(description, background_colour, get_glyph);
        printer.cleanup_previous_prints(font::MAX_HEIGHT, background_colour);
    }
};

extern App g_app;

#pragma once

#include "./tft.hpp"
#include "./font.hpp"
#include "./rgb565.hpp"
#include "./colour_functions.hpp"
#include "../scripts/glyphs/large_font.hpp"
#include "../scripts/glyphs/small_font.hpp"
#include <string.h>

#ifdef TEST_HARNESS
#include <format>
#include "./Arduino.hpp"
#else
#include <Arduino.h>
#define log_frame(label)
#endif

struct Digits {
    int16_t value;
    uint16_t absolute_value;
    static constexpr uint8_t TOTAL_DIGITS = 5;
    uint8_t digits[5];
    uint8_t leading_non_zero_digit_index;
    bool is_minus;
    Digits(int16_t _value) {
        value = _value;
        is_minus = value < 0;
        absolute_value = value & ~(1 << 15);
        leading_non_zero_digit_index = 0;
        uint16_t counter = absolute_value;
        for (uint8_t i = 0; i < TOTAL_DIGITS; i++) {
            const uint8_t digit = counter % 10;
            if (digit > 0) {
                leading_non_zero_digit_index = i;
            }
            digits[i] = digit;
            counter = (counter-digit) / 10;
        }
    }
    uint8_t operator[](uint8_t i) const {
        return digits[i];
    }
};

class App {
private:
    int16_t m_temperature_celcius;
    uint16_t m_humidity_percent;
    uint16_t m_time_24_hour;
    uint16_t m_wind_kph;
    struct {
        bool is_show_24_hour_time;
        bool is_show_leading_zero;
    } m_time_display_settings;
    struct {
        uint16_t time;
        uint16_t temperature;
        uint16_t humidity;
        uint16_t weather_description;
        uint16_t humidity_description;
        uint16_t wind_description;
        uint16_t moon_description;
    } m_y_text_end;
    struct {
        uint16_t time;
        uint16_t temperature;
        uint16_t humidity;
        void reset() {
            const uint16_t x_end = tft::SCREEN_WIDTH-1;
            time = x_end;
            temperature = x_end;
            humidity = x_end;
        }
    } m_x_text_end;
    struct {
        uint16_t weather_description;
        uint16_t humidity_description;
        uint16_t wind_description;
        uint16_t moon_description;
        void reset() {
            const uint16_t x_start = 0;
            weather_description = x_start;
            humidity_description = x_start;
            wind_description = x_start;
            moon_description = x_start;
        }
    } m_x_text_start;
    uint16_t m_x_margin;
    uint16_t m_y_margin;
    struct {
        RadialBackgroundColour freezing;
        RadialBackgroundColour cold;
        RadialBackgroundColour warm;
        RadialBackgroundColour hot;
    } m_background_colour;
    enum class BackgroundState: uint8_t {
        FREEZING = 0,
        COLD = 1,
        WARM = 2,
        HOT = 3,
    } m_background_state;
    struct {
        bool background;
        bool time;
        bool temperature;
        bool humidity;
        bool weather_description;
        bool humidity_description;
        bool wind_description;
        bool moon_description;
        void set_all(bool state) {
            background = state;
            time = state;
            temperature = state;
            humidity = state;
            weather_description = state;
            humidity_description = state;
            wind_description = state;
            moon_description = state;
        }
    } m_render_mask;
    rgb565_t m_text_colour;
public:
    App() {
        m_temperature_celcius = 0;
        m_humidity_percent = 0;
        m_time_24_hour = 0;
        m_wind_kph = 0;
        m_time_display_settings.is_show_24_hour_time = false;
        m_time_display_settings.is_show_leading_zero = true;

        m_background_colour.freezing.background_colour = create_rgb565_u8(40, 40, 80);
        m_background_colour.freezing.delta_colour = create_rgb565_bits(2,4,2);
        m_background_colour.cold.background_colour = create_rgb565_u8(30, 30, 80);
        m_background_colour.cold.delta_colour = create_rgb565_bits(1,1,2);
        m_background_colour.warm.background_colour = create_rgb565_u8(30, 80, 30);
        m_background_colour.warm.delta_colour = create_rgb565_bits(1,4,1);
        m_background_colour.hot.background_colour = create_rgb565_u8(80, 30, 30);
        m_background_colour.hot.delta_colour = create_rgb565_bits(2,1,1);
        m_background_state = BackgroundState::FREEZING;

        m_render_mask.set_all(true);

        m_text_colour = COLOUR.WHITE;

        m_x_margin = 10;
        m_y_margin = 10;
        {
            uint16_t y_text_end = 0;
            y_text_end += m_y_margin+large_font::MAX_HEIGHT;
            m_y_text_end.time = y_text_end-1;
            y_text_end += m_y_margin+large_font::MAX_HEIGHT;
            m_y_text_end.temperature = y_text_end-1;
            y_text_end += m_y_margin+large_font::MAX_HEIGHT;
            m_y_text_end.humidity = y_text_end-1;
            y_text_end += m_y_margin+small_font::MAX_HEIGHT;
            m_y_text_end.weather_description = y_text_end-1;
            y_text_end += m_y_margin+small_font::MAX_HEIGHT;
            m_y_text_end.humidity_description = y_text_end-1;
            y_text_end += m_y_margin+small_font::MAX_HEIGHT;
            m_y_text_end.wind_description = y_text_end-1;
            y_text_end += m_y_margin+small_font::MAX_HEIGHT;
            m_y_text_end.moon_description = y_text_end-1;
        }
        m_x_text_start.reset();
        m_x_text_end.reset();
        m_background_state = BackgroundState::FREEZING;
    }

    void render_all() {
        if (m_render_mask.background) {
            render_background();
            m_x_text_start.reset();
            m_x_text_end.reset();
        }
        if (m_render_mask.time) render_time();
        if (m_render_mask.temperature) render_temperature();
        if (m_render_mask.humidity) render_humidity();
        if (m_render_mask.weather_description) render_weather_description();
        if (m_render_mask.humidity_description) render_humidity_description();
        if (m_render_mask.wind_description) render_wind_description();
        if (m_render_mask.moon_description) render_moon_description();
        m_render_mask.set_all(false);
        log_frame(std::format("render_all: temperature={0}°C, humidity={1}%", m_temperature_celcius, m_humidity_percent));
    }

    void set_temperature(int16_t temperature_celcius) {
        const bool is_changed = m_temperature_celcius != temperature_celcius;
        m_render_mask.temperature |= is_changed;
        m_render_mask.weather_description |= is_changed;
        const auto new_background_state = get_background_state_from_temperature(temperature_celcius);
        if (new_background_state != m_background_state) {
            m_render_mask.set_all(true);
        }
        m_temperature_celcius = temperature_celcius;
        m_background_state = new_background_state;
    }

    void set_humidity(uint16_t humidity_percent) {
        const bool is_changed = m_humidity_percent != humidity_percent;
        m_render_mask.humidity |= is_changed;
        m_render_mask.humidity_description |= is_changed;
        m_humidity_percent = humidity_percent;
    }

    void set_time(uint16_t time_24_hour) {
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

    void set_wind(uint16_t wind_kph) {
        const bool is_changed = m_wind_kph != wind_kph;
        m_render_mask.wind_description |= is_changed;
        m_wind_kph = wind_kph;
    }
private:
    RadialBackgroundColour& get_background_colour() {
        switch (m_background_state) {
        case BackgroundState::FREEZING: return m_background_colour.freezing;
        case BackgroundState::COLD: return m_background_colour.cold;
        case BackgroundState::WARM: return m_background_colour.warm;
        case BackgroundState::HOT: return m_background_colour.hot;
        default: return m_background_colour.freezing;
        }
    }

    static BackgroundState get_background_state_from_temperature(int16_t temperature) {
        if (temperature < 0) return BackgroundState::FREEZING;
        if (temperature < 200) return BackgroundState::COLD;
        if (temperature < 300) return BackgroundState::WARM;
        return BackgroundState::HOT;
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
        RightToLeftPrinter printer;
        printer.x_end = tft::SCREEN_WIDTH-1-m_x_margin;
        printer.y_end = m_y_text_end.time;
        printer.text_colour = m_text_colour;
        uint16_t time = m_time_24_hour;
        if (!m_time_display_settings.is_show_24_hour_time) {
            if (time >= 2400) time -= 2400;
            const bool is_pm = time >= 1200;
            printer.print_string(is_pm ? "PM" : "AM", background_colour, get_glyph);
            if (time >= 1300) time -= 1200; // 13:00 -> 1:00PM
        }
        const auto digits = Digits(time);
        printer.print_char('0'+digits[0], background_colour, get_glyph);
        printer.print_char('0'+digits[1], background_colour, get_glyph);
        printer.print_char(':', background_colour, get_glyph);
        printer.print_char('0'+digits[2], background_colour, get_glyph);
        if (m_time_display_settings.is_show_leading_zero || digits[3] > 0) {
            printer.print_char('0'+digits[3], background_colour, get_glyph);
        }
        uint16_t& previous_x_end = m_x_text_end.time;
        if (previous_x_end < printer.x_end) {
            printer.pad_nothing(printer.x_end-previous_x_end, font::MAX_HEIGHT, background_colour);
        }
        previous_x_end = printer.x_end;
    }

    void render_temperature() {
        auto& background_colour = get_background_colour();
        namespace font = large_font;
        const auto get_glyph = &font::get_glyph;
        RightToLeftPrinter printer;
        printer.x_end = tft::SCREEN_WIDTH-1-m_x_margin;
        printer.y_end = m_y_text_end.temperature;
        printer.text_colour = m_text_colour;
        const auto digits = Digits(m_temperature_celcius);
        printer.print_char('C', background_colour, get_glyph);
        printer.print_char(0xB0, background_colour, get_glyph);
        printer.print_char('0'+digits[0], background_colour, get_glyph);
        printer.print_char('.', background_colour, get_glyph);
        printer.print_char('0'+digits[1], background_colour, get_glyph);
        if (digits.leading_non_zero_digit_index >= 2) printer.print_char('0'+digits[2], background_colour, get_glyph);
        if (digits.is_minus) printer.print_char('-', background_colour, get_glyph);
        const uint16_t curr_x_end = printer.x_end;
        uint16_t& previous_x_end = m_x_text_end.temperature;
        if (previous_x_end < curr_x_end) {
            printer.pad_nothing(curr_x_end-previous_x_end, font::MAX_HEIGHT, background_colour);
        }
        previous_x_end = curr_x_end;
    }

    void render_humidity() {
        auto& background_colour = get_background_colour();
        namespace font = large_font;
        const auto get_glyph = &font::get_glyph;
        RightToLeftPrinter printer;
        printer.x_end = tft::SCREEN_WIDTH-1-m_x_margin;
        printer.y_end = m_y_text_end.humidity;
        printer.text_colour = m_text_colour;
        const auto digits = Digits(m_humidity_percent);
        printer.print_char('%', background_colour, get_glyph);
        printer.print_char('0'+digits[0], background_colour, get_glyph);
        printer.print_char('.', background_colour, get_glyph);
        printer.print_char('0'+digits[1], background_colour, get_glyph);
        if (digits.leading_non_zero_digit_index >= 2) printer.print_char('0'+digits[2], background_colour, get_glyph);
        if (digits.leading_non_zero_digit_index >= 3) printer.print_char('0'+digits[3], background_colour, get_glyph);
        const uint16_t curr_x_end = printer.x_end;
        uint16_t& previous_x_end = m_x_text_end.humidity;
        if (previous_x_end < curr_x_end) {
            printer.pad_nothing(curr_x_end-previous_x_end, font::MAX_HEIGHT, background_colour);
        }
        previous_x_end = curr_x_end;
    }

    void render_weather_description() {
        auto& background_colour = get_background_colour();
        namespace font = small_font;
        const auto get_glyph = &font::get_glyph;
        LeftToRightPrinter printer;
        printer.x_start = m_x_margin;
        printer.y_end = m_y_text_end.weather_description;
        printer.text_colour = m_text_colour;
        const char* description = nullptr;
        if (m_temperature_celcius < 0) {
            description = "HEAVY SNOWSTORM";
        } else if (m_temperature_celcius < 200) {
            description = "STRONG OVERCAST";
        } else if (m_temperature_celcius < 300) {
            description = "SUNNY";
        } else {
            description = "INTENSE HEATWAVE";
        }
        if (description != nullptr) {
            printer.print_string(description, background_colour, get_glyph);
        }
        const uint16_t curr_x_start = printer.x_start;
        uint16_t& previous_x_start = m_x_text_start.weather_description;
        if (previous_x_start > curr_x_start) {
            printer.pad_nothing(previous_x_start-curr_x_start, font::MAX_HEIGHT, background_colour);
        }
        previous_x_start = curr_x_start;
    }

    void render_humidity_description() {
        auto& background_colour = get_background_colour();
        namespace font = small_font;
        const auto get_glyph = &font::get_glyph;
        LeftToRightPrinter printer;
        printer.x_start = m_x_margin;
        printer.y_end = m_y_text_end.humidity_description;
        printer.text_colour = m_text_colour;
        const char* description = nullptr;
        if (m_humidity_percent < 100) {
            description = "DRY AIR";
        } else if (m_humidity_percent < 300) {
            description = "MODERATE HUMIDITY";
        } else if (m_humidity_percent < 600) {
            description = "HEATSTROKE RISK";
        } else {
            description = "UNDERWATER";
        }
        if (description != nullptr) {
            printer.print_string(description, background_colour, get_glyph);
        }
        const uint16_t curr_x_start = printer.x_start;
        uint16_t& previous_x_start = m_x_text_start.humidity_description;
        if (previous_x_start > curr_x_start) {
            printer.pad_nothing(previous_x_start-curr_x_start, font::MAX_HEIGHT, background_colour);
        }
        previous_x_start = curr_x_start;
    }

    void render_wind_description() {
        auto& background_colour = get_background_colour();
        namespace font = small_font;
        const auto get_glyph = &font::get_glyph;
        LeftToRightPrinter printer;
        printer.x_start = m_x_margin;
        printer.y_end = m_y_text_end.wind_description;
        printer.text_colour = m_text_colour;
        printer.print_string("WIND: ", background_colour, get_glyph);
        const auto digits = Digits(m_wind_kph);
        int8_t leading_non_zero_digit_index = digits.leading_non_zero_digit_index;
        if (leading_non_zero_digit_index < 1) leading_non_zero_digit_index = 1;
        for (int8_t i = leading_non_zero_digit_index; i >= 0; i--) {
            const uint8_t digit = digits[i];
            if (i == 0) printer.print_char('.', background_colour, get_glyph);
            printer.print_char('0'+digit, background_colour, get_glyph);
        }
        printer.print_string("KPH", background_colour, get_glyph);
        const uint16_t curr_x_start = printer.x_start;
        uint16_t& previous_x_start = m_x_text_start.wind_description;
        if (previous_x_start > curr_x_start) {
            printer.pad_nothing(previous_x_start-curr_x_start, font::MAX_HEIGHT, background_colour);
        }
        previous_x_start = curr_x_start;
    }

    void render_moon_description() {
        auto& background_colour = get_background_colour();
        namespace font = small_font;
        const auto get_glyph = &font::get_glyph;
        LeftToRightPrinter printer;
        printer.x_start = m_x_margin;
        printer.y_end = m_y_text_end.moon_description;
        printer.text_colour = m_text_colour;
        printer.print_string("MOON: HALF MOON", background_colour, get_glyph);
        const uint16_t curr_x_start = printer.x_start;
        uint16_t& previous_x_start = m_x_text_start.moon_description;
        if (previous_x_start > curr_x_start) {
            printer.pad_nothing(previous_x_start-curr_x_start, font::MAX_HEIGHT, background_colour);
        }
        previous_x_start = curr_x_start;
    }
};


static void app_setup() {
    Serial.begin(9600);
    Serial.println("Starting up st7789 controller");
    tft::init();
    tft::set_brightness(50);
}

static App app;

static void app_loop() {
    app.set_time(1347);
    app.set_time_show_24_hour(true);
    app.set_time_show_leading_zero(true);
    app.set_temperature(-110);
    app.set_humidity(69);
    app.set_wind(257);
    app.render_all();
    delay(1000);

    app.set_time(1047);
    app.set_time_show_24_hour(true);
    app.set_time_show_leading_zero(true);
    app.set_temperature(95);
    app.set_humidity(215);
    app.set_wind(57);
    app.render_all();
    delay(1000);

    app.set_time(47);
    app.set_time_show_24_hour(false);
    app.set_time_show_leading_zero(false);
    app.set_temperature(217);
    app.set_humidity(419);
    app.set_wind(0);
    app.render_all();
    delay(1000);

    app.set_time(1547);
    app.set_time_show_24_hour(false);
    app.set_time_show_leading_zero(false);
    app.set_temperature(328);
    app.set_humidity(1000);
    app.set_wind(1017);
    app.render_all();
    delay(1000);

    constexpr uint8_t TOTAL_TIMES = 5;
    const uint16_t TEST_TIMES[TOTAL_TIMES] = { 1000, 100, 1230, 1890, 111 };
    for (uint8_t i = 0; i < TOTAL_TIMES; i++) {
        const uint16_t time = TEST_TIMES[i];
        app.set_time(time);
        app.render_all();
        delay(1000);
    }
    for (uint8_t i = 0; i <= 120; i+=1) {
        const uint16_t hours = i / 60;
        const uint16_t minutes = i - hours*60;
        const uint16_t time = 1300 + minutes + 100*hours;
        app.set_time(time);
        app.render_all();
    }
}

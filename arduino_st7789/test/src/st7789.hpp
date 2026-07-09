#pragma once
#include <optional>
#include <stdint.h>
#include <string>
#include <vector>
#include "./rgb565.hpp"

class ST7789
{
public:
    struct Rect {
        uint16_t x_start;
        uint16_t x_end;
        uint16_t y_start;
        uint16_t y_end;
    };

    struct Cursor {
        uint16_t x;
        uint16_t y;
    };

    enum class Mode: uint8_t {
        COMMAND = 0,
        DATA = 1,
    };
private:
    const uint16_t m_width;
    const uint16_t m_height;
    uint8_t m_brightness;
    uint8_t m_is_hardware_reset;
    uint64_t m_time_nanos;
    Mode m_mode;
    std::vector<rgb565_t> m_buffer;
    Rect m_rect;
    Cursor m_cursor;
public:
    ST7789(uint16_t width, uint16_t height)
    : m_width(width), m_height(height)
    {
        m_buffer.resize(width*height);
        m_brightness = 0;
        m_is_hardware_reset = 0;
        m_mode = Mode::COMMAND;
        m_time_nanos = 0;
        m_rect = {
            .x_start = 0,
            .x_end = 0,
            .y_start = 0,
            .y_end = 0,
        };
        m_cursor = {
            .x = 0,
            .y = 0,
        };
    }

    void set_brightness(uint8_t brightness) {
        m_brightness = brightness;
    }

    void hardware_reset() {
        m_is_hardware_reset = 1;
    }

    void set_write_rect(Rect rect) {
        if (rect.x_end >= m_width) {
            rect.x_end = m_width-1;
        }
        if (rect.y_end >= m_height) {
            rect.y_end = m_height-1;
        }
        if (rect.x_start > rect.x_end) {
            rect.x_start = rect.x_end;
        }
        if (rect.y_start > rect.y_end) {
            rect.y_start = rect.y_end;
        }
        m_rect = rect;
        m_cursor = {
            .x = rect.x_start,
            .y = rect.y_start,
        };
        m_mode = Mode::COMMAND;
    }

    void begin_write_pixel() {
        m_mode = Mode::DATA;
    }

    void end_write_pixel() {
        m_mode = Mode::COMMAND;
    }

    void sleep_nanos(uint64_t time_nanos) {
        m_time_nanos += time_nanos;
    }

    uint64_t get_time_nanos() const {
        return m_time_nanos;
    }

    void write(rgb565_t pixel) {
        // at f_spi = 8Mhz we have a clock period of 125ns
        // for 16bits this is 2us
        sleep_nanos(2000);

        if (m_mode != Mode::DATA) return;

        const uint32_t offset =
            static_cast<uint32_t>(m_cursor.y)*static_cast<uint32_t>(m_width) +
            static_cast<uint32_t>(m_cursor.x);
        m_cursor.x += 1;
        if (m_cursor.x > m_rect.x_end) {
            m_cursor.x = m_rect.x_start;
            m_cursor.y += 1;
        }
        if (m_cursor.y > m_rect.y_end) {
            m_cursor.y = m_rect.y_start;
        }
        m_buffer[offset] = pixel;
    }

    void debug_out(FILE* fp, std::optional<std::string> label);
};

#pragma once
#include <stdint.h>
#include <vector>
#include "./rgb565.hpp"

struct ST7789 {
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

    const uint16_t m_width;
    const uint16_t m_height;
    uint8_t m_brightness;
    uint8_t m_is_hardware_reset;
    Mode m_mode;
    std::vector<rgb565_t> m_buffer;
    Rect m_rect;
    Cursor m_cursor;
    bool m_x_mirror = false;
    bool m_y_mirror = false;

    ST7789(uint16_t width, uint16_t height)
    : m_width(width), m_height(height)
    {
        m_buffer.resize(width*height);
        m_brightness = 0;
        m_is_hardware_reset = 0;
        m_mode = Mode::COMMAND;
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

    void set_write_mode(bool x_mirror, bool y_mirror) {
        m_x_mirror = x_mirror;
        m_y_mirror = y_mirror;
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

    void write(rgb565_t pixel) {
        if (m_mode != Mode::DATA) return;

        uint16_t real_cursor_y = m_cursor.y;
        uint16_t real_cursor_x = m_cursor.x;
        if (m_x_mirror) {
            real_cursor_x = m_width-m_cursor.x-1;
        }
        if (m_y_mirror) {
            real_cursor_y = m_height-m_cursor.y-1;
        }
        const uint32_t offset =
            static_cast<uint32_t>(real_cursor_y)*static_cast<uint32_t>(m_width) +
            static_cast<uint32_t>(real_cursor_x);

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
};

extern ST7789 g_st7789;

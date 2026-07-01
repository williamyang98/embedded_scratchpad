#pragma once
#include <memory>
#include <optional>
#include <span>
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <vector>

typedef uint16_t rgb565_t;

// DOC: sitronix_st7789_datasheet.pdf
// Section 8.8.3: 8-bit data bus for 16-bit/pixel (RGB 5-6-5-bit input), 65K-Colors
// r = 32, g = 64, b = 32, rgb = 32*64*32 = 65536
static rgb565_t create_rgb565_bits(uint8_t r5, uint8_t g6, uint8_t b5) {
    uint16_t R = static_cast<uint16_t>(r5);
    uint16_t G = static_cast<uint16_t>(g6);
    uint16_t B = static_cast<uint16_t>(b5);
    R = (R & 0b011111) << 11;
    G = (G & 0b111111) << 5;
    B =  B & 0b011111;
    uint16_t RGB = R | G | B;
    return RGB;
}

static rgb565_t create_rgb565_u8(uint8_t r, uint8_t g, uint8_t b) {
    uint16_t R = (static_cast<uint16_t>(r) * 31) / 255;
    uint16_t G = (static_cast<uint16_t>(g) * 31) / 255;
    uint16_t B = (static_cast<uint16_t>(b) * 31) / 255;
    R = (R & 0b011111) << 11;
    G = (G & 0b111111) << 5;
    B =  B & 0b011111;
    uint16_t RGB = R | G | B;
    return RGB;
}

static rgb565_t create_rgb565_f32(float r, float g, float b) {
    uint16_t R = static_cast<uint16_t>(r*31.0f);
    uint16_t G = static_cast<uint16_t>(g*63.0f);
    uint16_t B = static_cast<uint16_t>(b*31.0f);
    R = (R & 0b011111) << 11;
    G = (G & 0b111111) << 5;
    B =  B & 0b011111;
    uint16_t RGB = R | G | B;
    return RGB;
}

const struct {
  rgb565_t BLACK   = create_rgb565_f32(0,0,0);
  rgb565_t RED     = create_rgb565_f32(1,0,0);
  rgb565_t GREEN   = create_rgb565_f32(0,1,0);
  rgb565_t BLUE    = create_rgb565_f32(0,0,1);
  rgb565_t CYAN    = create_rgb565_f32(0,1,1);
  rgb565_t MAGENTA = create_rgb565_f32(1,0,1);
  rgb565_t YELLOW  = create_rgb565_f32(1,1,0);
  rgb565_t WHITE   = create_rgb565_f32(1,1,1);
} COLOUR;


class Image {
private:
    const uint16_t m_width;
    const uint16_t m_height;
    const uint32_t m_total_pixels;
    std::vector<rgb565_t> m_data;
public:
    Image(uint16_t width, uint16_t height)
    :   m_width(width), m_height(height),
        m_total_pixels(static_cast<uint32_t>(width)*static_cast<uint32_t>(height))
    {
        m_data.resize(m_total_pixels);
    }

    uint16_t width() const { return m_width; }
    uint16_t height() const { return m_height; }
    uint32_t total_pixels() const { return m_total_pixels; }
    const std::span<const rgb565_t> data() const { return m_data; }
    std::span<rgb565_t> data() { return m_data; }
    rgb565_t& operator[](size_t index) { return m_data[index]; }
    const rgb565_t& operator[](size_t index) const { return m_data[index]; }
};

struct Rect {
    uint16_t x_start;
    uint16_t x_end;
    uint16_t y_start;
    uint16_t y_end;
    uint16_t width() const { return x_end-x_start+1; }
    uint16_t height() const { return y_end-y_start+1; }
    uint32_t size() const { return width()*height(); }
};

struct Cursor {
    uint16_t x;
    uint16_t y;
};

class ST7789
{
private:
    std::shared_ptr<Image> m_image;
    Rect m_rect;
    Cursor m_cursor;
public:
    ST7789(std::shared_ptr<Image> image)
    : m_image(image) {
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

    Image& image() { return *m_image.get(); }

    void set_write_rect(Rect rect) {
        if (rect.x_end >= m_image->width()) {
            rect.x_end = m_image->width()-1;
        }
        if (rect.y_end >= m_image->height()) {
            rect.y_end = m_image->height()-1;
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
    }

    void write(rgb565_t pixel) {
        auto& image = this->image();
        const uint32_t offset =
            static_cast<uint32_t>(m_cursor.y)*static_cast<uint32_t>(image.width()) +
            static_cast<uint32_t>(m_cursor.x);
        m_cursor.x += 1;
        if (m_cursor.x > m_rect.x_end) {
            m_cursor.x = m_rect.x_start;
            m_cursor.y += 1;
        }
        if (m_cursor.y > m_rect.y_end) {
            m_cursor.y = m_rect.y_start;
        }
        image[offset] = pixel;
    }

    void debug_out(FILE* fp, std::optional<std::string> label);
};


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <vector>
#include <array>
#include <memory>
#include <string>
#include <optional>
#include <format>
#include <span>

#if _WIN32
#define NOMINMAX
#include <io.h>
#include <fcntl.h>
#endif

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
    uint16_t R = static_cast<uint16_t>(r) * 31 / 255;
    uint16_t G = static_cast<uint16_t>(g) * 31 / 255;
    uint16_t B = static_cast<uint16_t>(b) * 31 / 255;
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

template <typename T>
T positive_mod(T x, T y) {
    T v = (x % y) % y;
    v = (v + y) % y;
    return v;
}

template <typename T>
size_t file_write_value(FILE* fp, T value) {
    return fwrite(reinterpret_cast<const void*>(&value), sizeof(T), 1, fp);
}

template <typename T>
size_t file_write_array(FILE* fp, std::span<const T> array) {
    return fwrite(reinterpret_cast<const void*>(array.data()), sizeof(T), array.size(), fp);
}


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

    void debug_out(FILE* fp, std::optional<std::string> label) {
        constexpr uint32_t MAGIC_NUMBER = 0xDEADBEEF;
        const std::array<const uint16_t, 8> HEADER = {
            m_rect.x_start, m_rect.x_end, m_rect.y_start, m_rect.y_end,
            m_cursor.x, m_cursor.y,
            m_image->width(), m_image->height() };
        file_write_value(fp, MAGIC_NUMBER);
        file_write_array<const uint16_t>(fp, HEADER);

        if (label.has_value()) {
            const uint32_t length = static_cast<uint32_t>(label.value().size());
            if (length > 0) {
                const auto data = std::span(
                    reinterpret_cast<const char*>(label.value().data()),
                    label.value().size()
                );
                file_write_value(fp, length);
                file_write_array(fp, data);
            }
        } else {
            const uint32_t length = 0;
            file_write_value(fp, length);
        }

        file_write_array(fp, std::span<const rgb565_t>(m_image->data()));
        fflush(fp);
    }
};

namespace gfx {
    void fill_screen(ST7789& screen, rgb565_t colour) {
        const auto image = screen.image();
        const Rect rect = {
            .x_start = 0,
            .x_end = uint16_t(image.width()-1),
            .y_start = 0,
            .y_end = uint16_t(image.height()-1),
        };
        const uint32_t total_pixels = rect.size();
        screen.set_write_rect(rect);
        for (uint32_t i = 0; i < total_pixels; i++) {
            screen.write(colour);
        }
    }

    void fill_rect(ST7789& screen, Rect rect, rgb565_t colour) {
        screen.set_write_rect(rect);
        const uint32_t total_pixels = rect.size();
        for (uint32_t i = 0; i < total_pixels; i++) {
            screen.write(colour);
        }
    }
};

int main(int argc, char** argv) {
    FILE* fp_in = stdin;
    FILE* fp_out = stdout;

#if _WIN32
    _setmode(_fileno(fp_in), _O_BINARY);
    _setmode(_fileno(fp_out), _O_BINARY);
#endif

    const uint16_t SCREEN_WIDTH = 240;
    const uint16_t SCREEN_HEIGHT = 280;

    auto image = std::make_shared<Image>(SCREEN_WIDTH, SCREEN_HEIGHT);
    auto st7789 = std::make_shared<ST7789>(image);
    auto screen = *st7789.get();

    gfx::fill_screen(screen, 0xFFFF);

    const std::array<rgb565_t, 8> TEST_COLOURS = {
        COLOUR.BLACK,
        COLOUR.RED,
        COLOUR.GREEN,
        COLOUR.BLUE,
        COLOUR.CYAN,
        COLOUR.MAGENTA,
        COLOUR.YELLOW,
        COLOUR.WHITE,
    };

    screen.debug_out(fp_out, "Initial frame");
    for (uint16_t i = 0; i < 64; i++) {
        const rgb565_t colour = TEST_COLOURS[i % TEST_COLOURS.size()];
        const uint16_t margin = i;
        const Rect rect = {
            .x_start = margin,
            .x_end = uint16_t(SCREEN_WIDTH-margin-1),
            .y_start = margin,
            .y_end = uint16_t(SCREEN_HEIGHT-margin-1),
        };
        gfx::fill_rect(screen, rect, colour);
        screen.debug_out(fp_out, std::format("Frame {}", i));
    }

    {
        gfx::fill_screen(screen, COLOUR.WHITE);
        const uint16_t width = 128;
        const uint16_t height = 128;
        const uint16_t x_start = 32;
        const uint16_t y_start = 32;
        const uint16_t x_end = x_start+width-1;
        const uint16_t y_end = y_start+height-1;
        const Rect rect = {
            .x_start = x_start,
            .x_end = x_end,
            .y_start = y_start,
            .y_end = y_end,
        };
        screen.set_write_rect(rect);
        for (uint16_t y = 0; y < height; y++) {
            for (uint16_t x = 0; x < width; x++) {
                const rgb565_t colour = create_rgb565_u8(x*2, 0, y*2);
                screen.write(colour);
            }
        }
        screen.debug_out(fp_out, "RGB square");
    }

    return 0;
}

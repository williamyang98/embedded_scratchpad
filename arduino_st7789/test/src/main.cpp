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
            .x_end = image.width(),
            .y_start = 0,
            .y_end = image.height(),
        };
        const int total_pixels = image.total_pixels();
        screen.set_write_rect(rect);
        for (int i = 0; i < total_pixels; i++) {
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
    auto st7789_ptr = std::make_shared<ST7789>(image);
    auto st7789 = *st7789_ptr.get();

    gfx::fill_screen(st7789, 0xFFFF);

    const std::array<rgb565_t, 8> TEST_COLOURS = {
        0b00000'000000'00000,
        0b11111'000000'00000,
        0b00000'111111'00000,
        0b00000'000000'11111,
        0b11111'000000'11111,
        0b00000'111111'11111,
        0b11111'111111'00000,
        0b11111'111111'11111,
    };

    st7789.debug_out(fp_out, "Initial frame");
    for (uint16_t i = 0; i < 64; i++) {
        const rgb565_t colour = TEST_COLOURS[i % TEST_COLOURS.size()];
        const uint16_t margin = i;

        const Rect rect = {
            .x_start = margin,
            .x_end = uint16_t(SCREEN_WIDTH-margin-1),
            .y_start = margin,
            .y_end = uint16_t(SCREEN_HEIGHT-margin-1),
        };
        const uint32_t rect_size = rect.size();
        st7789.set_write_rect(rect);
        for (uint32_t j = 0; j < rect_size; j++) {
            st7789.write(colour);
        }
        st7789.debug_out(fp_out, std::format("Frame {}", i));
    }

    return 0;
}

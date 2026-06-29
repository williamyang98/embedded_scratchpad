#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <vector>
#include <array>
#include <memory>
#include <cmath>
#include <string>
#include <optional>
#include <format>

#if _WIN32
#define NOMINMAX
#include <io.h>
#include <fcntl.h>
#endif

typedef uint16_t RGB565 ;

class Image {
private:
    const int m_width;
    const int m_height;
    const int m_total_pixels;
    std::vector<RGB565> m_data;
public:
    Image(int width, int height)
    : m_width(width), m_height(height), m_total_pixels(width*height), m_data(width*height)
    {}

    int width() const { return m_width; }
    int height() const { return m_height; }
    int total_pixels() const { return m_total_pixels; }
    const RGB565* data() const { return m_data.data(); }
    RGB565* data() { return m_data.data(); }
    RGB565& operator[](size_t index) { return m_data[index]; }
    const RGB565& operator[](size_t index) const { return m_data[index]; }
};

struct Rect {
    int x_start;
    int x_end;
    int y_start;
    int y_end;
    int width() const { return std::abs(x_end-x_start); }
    int height() const { return std::abs(y_end-y_start); }
    int size() const { return width()*height(); }
};

template <typename T>
T positive_mod(T x, T y) {
    T v = (x % y) % y;
    v = (v + y) % y;
    return v;
}

class ST7789
{
private:
    std::shared_ptr<Image> m_image;
    Rect m_rect;
    int m_rect_width;
    int m_rect_height;
    int m_rect_size;
    int m_index;
public:
    ST7789(std::shared_ptr<Image> image)
    : m_image(image) {
        m_rect = {
            .x_start = 0,
            .x_end = 0,
            .y_start = 0,
            .y_end = 0,
        };
        m_index = 0;
    }

    Image& image() { return *m_image.get(); }

    void set_write_rect(Rect rect) {
        // rect.x_start = positive_mod(rect.x_start, m_image->width());
        // rect.x_end = positive_mod(rect.x_end, m_image->width());
        // rect.y_start = positive_mod(rect.y_start, m_image->height());
        // rect.y_end = positive_mod(rect.y_end, m_image->height());
        m_rect = rect;
        m_rect_width = rect.width();
        m_rect_height = rect.height();
        m_rect_size = rect.size();
        m_index = 0;
    }

    void write(RGB565 pixel) {
        if (m_rect_size == 0) return;
        Image& image = *m_image.get();
        const int x_rect = m_index % m_rect_width;
        const int y_rect = (m_index / m_rect_width) % m_rect_height;
        const int y_image = (m_rect.y_start + y_rect) % image.height();
        const int offset = (y_image*image.width() + m_rect.x_start + x_rect) % image.total_pixels();
        image[offset] = pixel;
        m_index = (m_index + 1) % m_rect_size;
    }

    void debug_out(FILE* fp, std::optional<std::string> label) {
        constexpr int MAGIC_NUMBER = static_cast<int>(0xDEADBEEF);
        const int HEADER[] = { MAGIC_NUMBER, m_rect.x_start, m_rect.x_end, m_rect.y_start, m_rect.y_end, m_index, m_image->width(), m_image->height() };
        fwrite(reinterpret_cast<const void*>(HEADER), sizeof(int), sizeof(HEADER)/sizeof(int), fp);
        if (label.has_value()) {
            const uint32_t length = static_cast<uint32_t>(label.value().size());
            if (length > 0) {
                const char* data = label.value().data();
                fwrite(reinterpret_cast<const void*>(&length), sizeof(uint32_t), 1, fp);
                fwrite(reinterpret_cast<const void*>(data), sizeof(char), length, fp);
            }
        } else {
            const uint32_t length = 0;
            fwrite(reinterpret_cast<const void*>(&length), sizeof(uint32_t), 1, fp);
        }

        fwrite(reinterpret_cast<const void*>(m_image->data()), sizeof(RGB565), m_image->total_pixels(), fp);
        fflush(fp);
    }
};

namespace gfx {
    void fill_screen(ST7789& screen, RGB565 colour) {
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

    const int SCREEN_WIDTH = 240;
    const int SCREEN_HEIGHT = 280;

    auto image = std::make_shared<Image>(SCREEN_WIDTH, SCREEN_HEIGHT);
    auto st7789_ptr = std::make_shared<ST7789>(image);
    auto st7789 = *st7789_ptr.get();

    gfx::fill_screen(st7789, 0xFFFF);

    const std::array<RGB565, 8> TEST_COLOURS = {
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
    for (int i = 0; i < 64; i++) {
        const RGB565 colour = TEST_COLOURS[i % TEST_COLOURS.size()];
        const int j = i;

        const Rect rect = { .x_start = j, .x_end = SCREEN_WIDTH-j, .y_start = j, .y_end = SCREEN_HEIGHT-j };
        const int rect_size = rect.size();
        st7789.set_write_rect(rect);
        for (int j = 0; j < rect_size; j++) {
            st7789.write(colour);
        }
        st7789.debug_out(fp_out, std::format("Frame {}", i));
    }

    return 0;
}

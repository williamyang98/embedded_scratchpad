#include "./st7789.hpp"
#include <array>
#include <span>
#include <stdlib.h>

template <typename T>
static size_t file_write_value(FILE* fp, T value) {
    return fwrite(reinterpret_cast<const void*>(&value), sizeof(T), 1, fp);
}

template <typename T>
static size_t file_write_array(FILE* fp, std::span<const T> array) {
    return fwrite(reinterpret_cast<const void*>(array.data()), sizeof(T), array.size(), fp);
}

void ST7789::debug_out(FILE* fp, std::optional<std::string> label) {
    constexpr uint32_t MAGIC_NUMBER = 0xDEADBEEF;
    const std::array<const uint16_t, 8> HEADER = {
        m_rect.x_start, m_rect.x_end, m_rect.y_start, m_rect.y_end,
        m_cursor.x, m_cursor.y,
        m_width, m_height };

    file_write_value(fp, MAGIC_NUMBER);
    file_write_array<const uint16_t>(fp, HEADER);
    file_write_value(fp, m_brightness);
    file_write_value(fp, m_is_hardware_reset);
    m_is_hardware_reset = 0;

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

    file_write_array(fp, std::span<const rgb565_t>(m_buffer));
    fflush(fp);
}

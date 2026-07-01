#include "./st7789.hpp"
#include "./utility.hpp"
#include <array>

void ST7789::debug_out(FILE* fp, std::optional<std::string> label) {
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

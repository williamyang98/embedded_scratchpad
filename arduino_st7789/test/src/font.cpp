#include "./font.hpp"
#include <stdint.h>
#include <array>
#include <span>
#include <optional>

static const auto DIGIT_0_RLE = std::to_array<const uint8_t>({
    #include "../scripts/glyphs/0.hex"
});

static const auto DIGIT_1_RLE = std::to_array<const uint8_t>({
    #include "../scripts/glyphs/1.hex"
});

static const auto DIGIT_2_RLE = std::to_array<const uint8_t>({
    #include "../scripts/glyphs/2.hex"
});

static const auto DIGIT_3_RLE = std::to_array<const uint8_t>({
    #include "../scripts/glyphs/3.hex"
});

static const auto DIGIT_4_RLE = std::to_array<const uint8_t>({
    #include "../scripts/glyphs/4.hex"
});

static const auto DIGIT_5_RLE = std::to_array<const uint8_t>({
    #include "../scripts/glyphs/5.hex"
});

static const auto DIGIT_6_RLE = std::to_array<const uint8_t>({
    #include "../scripts/glyphs/6.hex"
});

static const auto DIGIT_7_RLE = std::to_array<const uint8_t>({
    #include "../scripts/glyphs/7.hex"
});

static const auto DIGIT_8_RLE = std::to_array<const uint8_t>({
    #include "../scripts/glyphs/8.hex"
});

static const auto DIGIT_9_RLE = std::to_array<const uint8_t>({
    #include "../scripts/glyphs/9.hex"
});

static const auto DIGIT_C_RLE = std::to_array<const uint8_t>({
    #include "../scripts/glyphs/C.hex"
});

static const auto DIGIT_F_RLE = std::to_array<const uint8_t>({
    #include "../scripts/glyphs/F.hex"
});


namespace gfx {

static std::optional<std::span<const uint8_t>> get_digit_rle_data(char digit) {
    switch (digit) {
    case '0': return DIGIT_0_RLE;
    case '1': return DIGIT_1_RLE;
    case '2': return DIGIT_2_RLE;
    case '3': return DIGIT_3_RLE;
    case '4': return DIGIT_4_RLE;
    case '5': return DIGIT_5_RLE;
    case '6': return DIGIT_6_RLE;
    case '7': return DIGIT_7_RLE;
    case '8': return DIGIT_8_RLE;
    case '9': return DIGIT_9_RLE;
    case 'C': return DIGIT_C_RLE;
    case 'F': return DIGIT_F_RLE;
    default: return std::nullopt;
    }
}

static rgb565_t blend_rgb565_u4(rgb565_t colour_0, rgb565_t colour_1, uint8_t alpha) {
    const uint8_t r_0 = (colour_0 & 0b11111'000000'00000) >> 11;
    const uint8_t g_0 = (colour_0 & 0b00000'111111'00000) >> 5;
    const uint8_t b_0 =  colour_0 & 0b00000'000000'11111;
    const uint8_t r_1 = (colour_1 & 0b11111'000000'00000) >> 11;
    const uint8_t g_1 = (colour_1 & 0b00000'111111'00000) >> 5;
    const uint8_t b_1 =  colour_1 & 0b00000'000000'11111;

    const uint8_t beta = 3-alpha;
    const uint8_t r = (r_0*alpha + r_1*beta)/3;
    const uint8_t g = (g_0*alpha + g_1*beta)/3;
    const uint8_t b = (b_0*alpha + b_1*beta)/3;

    const uint16_t R = (static_cast<uint16_t>(r) & 0b011111) << 11;
    const uint16_t G = (static_cast<uint16_t>(g) & 0b111111) << 5;
    const uint16_t B =  static_cast<uint16_t>(b) & 0b011111;
    return R | G | B;
}

class BitReader {
private:
    uint8_t curr_bit = 0;
    uint16_t curr_byte = 0;
    std::span<const uint8_t> data;
public:
    BitReader(std::span<const uint8_t> _data): data(_data) {}
    uint8_t read_bits(uint8_t n_bits) {
        const uint8_t remaining_bits = 8-curr_bit;
        const uint8_t shift_bits = n_bits > remaining_bits ? remaining_bits : n_bits;
        const uint8_t mask = ~(0xFF << shift_bits);
        uint8_t bits = (data[curr_byte] >> curr_bit) & mask;

        curr_bit += shift_bits;
        if (curr_bit == 8) {
            curr_bit = 0;
            curr_byte += 1;
        }

        n_bits -= shift_bits;
        if (n_bits > 0) {
            const uint8_t bits_high = read_bits(n_bits);
            bits |= bits_high << shift_bits;
        }
        return bits;
    }
};

bool write_digit(ST7789& screen, char digit, uint16_t x_start, uint16_t y_start, rgb565_t text_colour, rgb565_t background_colour) {
    const auto data_opt = get_digit_rle_data(digit);
    if (!data_opt.has_value()) return false;
    const auto data = data_opt.value();

    const uint16_t width = static_cast<uint16_t>(data[0]);
    const uint16_t height = static_cast<uint16_t>(data[1]);

    const Rect rect = {
        .x_start = x_start,
        .x_end = static_cast<uint16_t>(x_start+width-1),
        .y_start = y_start,
        .y_end = static_cast<uint16_t>(y_start+height-1),
    };

    const uint16_t total_pixels = width*height;

    screen.set_write_rect(rect);

    uint16_t curr_pixel = 0;
    auto bit_reader = BitReader(data.subspan(2));

    while (curr_pixel < total_pixels) {
        const uint8_t value = bit_reader.read_bits(2);
        const rgb565_t colour = blend_rgb565_u4(text_colour, background_colour, value);
        if (value == 0 || value == 3) {
            const uint8_t length = bit_reader.read_bits(8);
            for (uint16_t i = 0; i < length; i++) {
                screen.write(colour);
            }
            curr_pixel += static_cast<uint16_t>(length);
        } else {
            curr_pixel += 1;
            screen.write(colour);
        }
    }


    return true;
}

};

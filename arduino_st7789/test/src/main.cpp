#include <stdio.h>
#include <stdint.h>
#include <array>
#include <memory>
#include <optional>
#include <format>
#include "./st7789.hpp"
#include "./gfx.hpp"
#include "./font.hpp"

#if _WIN32
#define NOMINMAX
#include <io.h>
#include <fcntl.h>
#endif


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
    for (uint16_t i = 0; i < 16; i++) {
        const rgb565_t colour = TEST_COLOURS[i % TEST_COLOURS.size()];
        const uint16_t margin = i*4;
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

    {
        const std::string glyphs = "0123456789CF";
        for (const char glyph: glyphs) {
            const auto background_colour = COLOUR.BLUE;
            const auto font_color = COLOUR.WHITE;
            gfx::fill_screen(screen, background_colour);
            gfx::write_digit(screen, glyph, 32, 32, font_color, background_colour);
            screen.debug_out(fp_out, std::format("Glyph {0}", glyph));
        }
    }

    return 0;
}

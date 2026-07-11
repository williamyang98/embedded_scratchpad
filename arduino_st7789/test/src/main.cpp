#include <stdint.h>
#include "./sketch.hpp"

HardwareSerial Serial; // extern
ST7789 g_st7789(tft::SCREEN_WIDTH, tft::SCREEN_HEIGHT); // extern
App g_app; // extern
CommandParser g_command_parser; // extern
ResponseSender g_response_sender; // extern
GlyphRGBAQ256PaletteRenderSettings g_glyph_rgba_q256_palette_render_settings; // extern

int main(int argc, char** argv) {
    sketch_setup();
    while (true) {
        sketch_loop();
        break;
    }
    return 0;
}

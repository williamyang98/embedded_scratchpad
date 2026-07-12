#include "./src/sketch.hpp"

App g_app; // extern
CommandParser g_command_parser; // extern
ResponseSender g_response_sender; // extern
GlyphRGBAQ256PaletteRenderSettings g_glyph_rgba_q256_palette_render_settings; // extern

void setup(void) {
  sketch_setup();
}

void loop() {
  sketch_loop();
}
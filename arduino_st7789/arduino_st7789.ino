#include "./test/src/sketch.hpp"

App app; // extern
CommandParser command_parser; // extern
ResponseSender response_sender; // extern
GlyphRGBAQ256PaletteRenderSettings glyph_rgba_q256_palette_render_settings; // extern

void setup(void) {
  sketch_setup();
}

void loop() {
  sketch_loop();
}
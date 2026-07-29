#pragma once

#include "./hardware/serial.hpp"
#include "./hardware/tft.hpp"
#include "./hardware/pgmspace.h"
#include "./app/app.hpp"
#include "./app/commands.hpp"
#include "./app/response.hpp"

static void sketch_setup() {
    Serial.begin(9600);
    g_response_sender.send_message(FLASH_STRING("Initating ST7789 sketch"));
    tft::init();
    tft::set_brightness(50);
    tft::set_write_mode(false, false);
    g_glyph_rgba_q256_palette_render_settings.x_mirror = false;
    g_glyph_rgba_q256_palette_render_settings.y_mirror = false;
    g_app.render_all();
    g_app.set_page(AppPage::WEATHER_PAGE);
}

static void sketch_loop() {
    while (true) {
        const int result = Serial.read();
        if (result == -1) break;
        const uint8_t data = static_cast<uint8_t>(result);
        g_command_parser.read_incoming_byte(data);
    }
}

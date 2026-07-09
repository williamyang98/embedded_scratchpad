#pragma once

#include "./Arduino.hpp"
#include "./app.hpp"
#include "./commands.hpp"

extern App app;
extern CommandParser command_parser;

static void sketch_setup() {
    Serial.begin(9600);
    Serial.println(F("Starting up st7789 controller"));
    tft::init();
    tft::set_brightness(50);
}

static void sketch_loop() {
    app.render_all();

    while (true) {
        const int result = Serial.read();
        if (result == -1) break;
        const uint8_t data = static_cast<uint8_t>(result);
        command_parser.read_incoming_byte(data);
    }
}

#include <stdint.h>
#include "./sketch.hpp"

HardwareSerial Serial; // extern
ST7789 st7789(tft::SCREEN_WIDTH, tft::SCREEN_HEIGHT); // extern
App app; // extern
CommandParser command_parser; // extern
ResponseSender response_sender; // extern

int main(int argc, char** argv) {
    sketch_setup();
    while (true) {
        sketch_loop();
        break;
    }
    return 0;
}

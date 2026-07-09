#include "./test/src/sketch.hpp"

App app; // extern
CommandParser command_parser; // extern
ResponseSender response_sender; // extern

void setup(void) {
  sketch_setup();
}

void loop() {
  sketch_loop();
}
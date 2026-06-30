#include "tft.hpp"

void setup(void) {
  Serial.begin(9600);
  Serial.println("Custom library test");
  tft::init();
  tft::set_brightness(50);
}

constexpr int TOTAL_TEST_COLOURS = 8;
static tft::rgb565_t TEST_COLOURS[TOTAL_TEST_COLOURS] = {
  tft::COLOUR.BLACK,
  tft::COLOUR.RED,
  tft::COLOUR.GREEN,
  tft::COLOUR.BLUE,
  tft::COLOUR.CYAN,
  tft::COLOUR.MAGENTA,
  tft::COLOUR.YELLOW,
  tft::COLOUR.WHITE,
};

void loop() {
  int loop_size = 1;
  uint32_t millis_start = millis();

  for (int i = 0; i < loop_size; i++) {
    for (int i = 0; i < TOTAL_TEST_COLOURS; i++) {
      tft::fill_screen(TEST_COLOURS[i]);
    }
  }

  uint32_t millis_end = millis();
  uint32_t millis_elapsed = millis_end-millis_start;
  Serial.println(millis_elapsed);
}
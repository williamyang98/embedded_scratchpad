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
#if 0
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
#else
  static int i = 0;
  i += 1;
  const tft::rgb565_t background_colour = i % 2 == 0 ? tft::COLOUR.BLACK : tft::COLOUR.WHITE;
  tft::fill_screen(background_colour);
  {
    int16_t width = 129;
    int16_t height = 129;
    int16_t x_start = 240-129-1;
    int16_t x_end = x_start+width;
    int16_t y_start = 280-129-1;
    int16_t y_end = y_start+height;
    tft::set_write_rect(x_start, x_end, y_start, y_end);
    tft::begin_write_pixel();

    int16_t circle_radius = 65;
    int16_t circle_radius_squared = circle_radius*circle_radius;
    tft::rgb565_t circle_colour = tft::COLOUR.RED;
    for (uint8_t y = 0; y <= height; y++) {
      int16_t dy = abs(int16_t(y)-int16_t((height+1)/2));
      int16_t dy_squared = dy*dy;
      for (uint8_t x = 0; x <= width; x++) {
        //tft::rgb565_t colour = tft::create_rgb565_colour(x/2, (x+y)/2, y/2);
        //tft::rgb565_t colour = tft::create_rgb565_colour(y/4, 0, 0);
        //tft::rgb565_t colour = tft::create_rgb565_colour(x/4, 0, y/4);
        int16_t dx = abs(int16_t(x)-int16_t((width+1)/2));
        int16_t dx_squared = dx*dx;
        int16_t radius_squared = dx_squared + dy_squared;
        tft::rgb565_t colour = radius_squared <= circle_radius_squared ? circle_colour : background_colour;
        tft::write_pixel(colour);
      }
    }
    tft::end_write_pixel();
  }
  delay(1000);
#endif 
}
#include "./tft.hpp"
#include "./font.hpp"
#include <avr/pgmspace.h>
#include "./test/scripts/glyphs/space_grotesk_medium.hpp"

void setup(void) {
  Serial.begin(9600);
  Serial.println("Custom library test");
  tft::init();
  tft::set_brightness(50);
}

const struct {
  tft::rgb565_t BLACK   = tft::create_rgb565_f32(0,0,0);
  tft::rgb565_t RED     = tft::create_rgb565_f32(1,0,0);
  tft::rgb565_t GREEN   = tft::create_rgb565_f32(0,1,0);
  tft::rgb565_t BLUE    = tft::create_rgb565_f32(0,0,1);
  tft::rgb565_t CYAN    = tft::create_rgb565_f32(0,1,1);
  tft::rgb565_t MAGENTA = tft::create_rgb565_f32(1,0,1);
  tft::rgb565_t YELLOW  = tft::create_rgb565_f32(1,1,0);
  tft::rgb565_t WHITE   = tft::create_rgb565_f32(1,1,1);
} COLOUR;

constexpr int TOTAL_TEST_COLOURS = 8;
static tft::rgb565_t TEST_COLOURS[TOTAL_TEST_COLOURS] = {
  COLOUR.BLACK,
  COLOUR.RED,
  COLOUR.GREEN,
  COLOUR.BLUE,
  COLOUR.CYAN,
  COLOUR.MAGENTA,
  COLOUR.YELLOW,
  COLOUR.WHITE,
};

void loop() {
#define _TEST 2
#if _TEST == 0
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
#elif _TEST == 1
  static int i = 0;
  i += 1;
  const tft::rgb565_t background_colour = i % 2 == 0 ? COLOUR.BLACK : COLOUR.WHITE;
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
    tft::rgb565_t circle_colour = COLOUR.RED;
    for (uint8_t y = 0; y <= height; y++) {
      int16_t dy = abs(int16_t(y)-int16_t((height+1)/2));
      int16_t dy_squared = dy*dy;
      for (uint8_t x = 0; x <= width; x++) {
        //tft::rgb565_t colour = tft::create_rgb565_u8(x/2, (x+y)/2, y/2);
        //tft::rgb565_t colour = tft::create_rgb565_u8(y/4, 0, 0);
        //tft::rgb565_t colour = tft::create_rgb565_u8(x/4, 0, y/4);
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
#elif _TEST == 2
  static bool is_text_black = true;
  static char digit = 0;
  static uint16_t x_start = 0;
  if (digit == 0) {
    is_text_black = !is_text_black;
    x_start = 0;
    delay(1000);
  }
  const tft::rgb565_t background_colour = is_text_black ? COLOUR.WHITE : COLOUR.BLACK;
  const tft::rgb565_t text_colour = is_text_black ? COLOUR.BLACK : COLOUR.WHITE;
  if (digit == 0) {
    tft::fill_screen(background_colour);
  }
  const auto glyph = space_grotesk_medium::get_glyph(digit);
  if (glyph != nullptr) {
    gfx::write_glyph(*glyph, x_start, 32, text_colour, background_colour);
    x_start += glyph->width;
  }
  digit += 1;
#elif _TEST == 3
  static int i = 0;
  i += 1;
  const tft::rgb565_t background_colour = i % 2 == 0 ? COLOUR.BLACK : COLOUR.WHITE;
  tft::fill_screen(background_colour);
  {
    const uint16_t width = 128;
    const uint16_t height = 128;
    const uint16_t x_start = 32;
    const uint16_t y_start = 32;
    const uint16_t x_end = x_start+width-1;
    const uint16_t y_end = y_start+height-1;
    tft::set_write_rect(x_start, x_end, y_start, y_end);
    tft::begin_write_pixel();
    for (uint8_t y = 0; y < height; y++) {
      for (uint8_t x = 0; x < width; x++) {
        tft::rgb565_t colour = tft::create_rgb565_u8(x*2, 0, y*2);
        tft::write_pixel(colour);
      }
    }
    tft::end_write_pixel();
  }
  delay(1000);
#endif 
}
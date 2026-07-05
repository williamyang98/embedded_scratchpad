#pragma once

#include "./tft.hpp"
#include "./font.hpp"
#include "./rgb565.hpp"
#include "./colour_functions.hpp"
#include "../scripts/glyphs/large_font.hpp"
#include "../scripts/glyphs/small_font.hpp"
#include <string.h>

#ifdef TEST_HARNESS
#include <format>
#include "./Arduino.hpp"
#else
#include <Arduino.h>
#define log_frame(label)
#endif

constexpr int TOTAL_TEST_COLOURS = 8;
static rgb565_t TEST_COLOURS[TOTAL_TEST_COLOURS] = {
    COLOUR.BLACK,
    COLOUR.RED,
    COLOUR.GREEN,
    COLOUR.BLUE,
    COLOUR.CYAN,
    COLOUR.MAGENTA,
    COLOUR.YELLOW,
    COLOUR.WHITE,
};

static void test_screen_colours() {
    const uint32_t millis_start = millis();
    for (int i = 0; i < TOTAL_TEST_COLOURS; i++) {
        tft::fill_screen(TEST_COLOURS[i]);
        log_frame(std::format("Testing full screen color {0}", i));
    }
    const uint32_t millis_end = millis();
    const uint32_t millis_elapsed = millis_end-millis_start;
    Serial.println(millis_elapsed);
}

static void test_tunnel_colours() {
    uint16_t x_start = 0;
    uint16_t x_end = tft::SCREEN_WIDTH-1;
    uint16_t y_start = 0;
    uint16_t y_end = tft::SCREEN_HEIGHT-1;
    const uint16_t step = 4;
    for (uint8_t i = 0; i < 32; i++) {
        const rgb565_t colour = TEST_COLOURS[i % TOTAL_TEST_COLOURS];
        tft::fill_rect(x_start, x_end, y_start, y_end, colour);
        log_frame(std::format("Testing tunnel colour {0}", i));
        x_start += step;
        y_start += step;
        x_end -= step;
        y_end -= step;
    }
}

static void test_circle() {
    const rgb565_t background_colour = COLOUR.BLACK;
    tft::fill_screen(background_colour);

    const int16_t width = 129;
    const int16_t height = 129;
    const int16_t x_start = 240-129-1;
    const int16_t x_end = x_start+width;
    const int16_t y_start = 280-129-1;
    const int16_t y_end = y_start+height;
    tft::set_write_rect(x_start, x_end, y_start, y_end);

    const int16_t circle_radius = 65;
    const int16_t circle_radius_squared = circle_radius*circle_radius;
    const rgb565_t circle_colour = COLOUR.RED;
    tft::begin_write_pixel();
    for (uint8_t y = 0; y <= height; y++) {
        const int16_t dy = abs(int16_t(y)-int16_t((height+1)/2));
        const int16_t dy_squared = dy*dy;
        for (uint8_t x = 0; x <= width; x++) {
            //rgb565_t colour = create_rgb565_u8(x/2, (x+y)/2, y/2);
            //rgb565_t colour = create_rgb565_u8(y/4, 0, 0);
            //rgb565_t colour = create_rgb565_u8(x/4, 0, y/4);
            const int16_t dx = abs(int16_t(x)-int16_t((width+1)/2));
            const int16_t dx_squared = dx*dx;
            const int16_t radius_squared = dx_squared + dy_squared;
            const rgb565_t colour = radius_squared <= circle_radius_squared ? circle_colour : background_colour;
            tft::write_pixel(colour);
        }
    }
    tft::end_write_pixel();
    log_frame("Test circle");
    delay(1000);
}

static void test_rgb_square() {
    const rgb565_t background_colour = COLOUR.WHITE;
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
                rgb565_t colour = create_rgb565_u8(x*2, 0, y*2);
                tft::write_pixel(colour);
            }
        }
        tft::end_write_pixel();
    }
    log_frame("Testing rgb square");
    delay(1000);
}

static void test_glyphs_with_solid_background() {
    const auto background_colour = COLOUR.BLACK;
    const auto font_color = COLOUR.WHITE;
    tft::fill_screen(background_colour);
    const uint16_t y_offset = 20;
    uint16_t x_start = 0;
    uint16_t y_start = y_offset;
    uint16_t x_margin = 0;
    uint16_t y_margin = 2;
    uint16_t total_glyphs = 0;
    for (uint8_t c = 0; c < 255; c++) {
        const auto glyph = large_font::get_glyph(c);
        if (glyph == nullptr) continue;
        const uint16_t x_end = x_start + glyph->width + x_margin;
        if (x_end >= tft::SCREEN_WIDTH) {
            x_start = 0;
            y_start = y_start + glyph->height + y_margin;
        }
        const uint16_t y_end = y_start + glyph->height + y_margin;
        if (y_end >= tft::SCREEN_HEIGHT) {
            y_start = y_offset;
        }
        write_glyph(*glyph, x_start, y_start, font_color, SolidBackgroundColour(background_colour));
        x_start += glyph->width + x_margin;
        total_glyphs += 1;
    }
    log_frame(std::format("Font with {0} glyphs on solid background", total_glyphs));
    delay(1000);
}


static void test_glyphs_with_radial_background() {
    RadialBackgroundColour radial;
    for (int i = 0; i < 3; i++) {
        if (i % 3 == 0) {
            radial.background_colour = create_rgb565_u8(30, 30, 80);
            radial.delta_colour = create_rgb565_bits(1,1,2);
        } else if (i % 3 == 1) {
            radial.background_colour = create_rgb565_u8(30, 80, 30);
            radial.delta_colour = create_rgb565_bits(1,4,1);
        } else {
            radial.background_colour = create_rgb565_u8(80, 30, 30);
            radial.delta_colour = create_rgb565_bits(2,1,1);
        }

        radial.x_start = 0;
        radial.y_start = 0;
        radial.x_end = tft::SCREEN_WIDTH-1;
        radial.y_end = tft::SCREEN_HEIGHT-1;
        radial.fill();

        const uint16_t y_offset = 20;
        uint16_t x_start = 0;
        uint16_t y_start = y_offset;
        const uint16_t x_margin = 0;
        const uint16_t y_margin = 2;
        const rgb565_t text_colour = COLOUR.WHITE;
        uint8_t total_glyphs = 0;
        for (uint8_t c = 0; c < 255; c++) {
            const auto glyph = large_font::get_glyph(c);
            if (glyph == nullptr) continue;
            total_glyphs += 1;
            const uint16_t x_end = x_start + glyph->width + x_margin - 1;
            if (x_end > tft::SCREEN_WIDTH-1) {
                x_start = 0;
                y_start += glyph->height + y_margin;
            }
            const uint16_t y_end = y_start + glyph->height + y_margin - 1;
            if (y_end > tft::SCREEN_HEIGHT-1) {
                y_start = y_offset;
            }
            radial.x_start = x_start;
            radial.y_start = y_start;
            radial.x_end = x_start + glyph->width - 1;
            radial.y_end = y_start + glyph->height - 1;
            radial.reset_cursor();
            write_glyph(*glyph, x_start, y_start, text_colour, radial);
            x_start += glyph->width;
        }
        log_frame(std::format("Font with {0} glyphs on radial background {1}", total_glyphs, i));
        delay(1000);
    }
}

struct DecimalValue {
    int16_t value;
    uint16_t absolute_value;
    uint8_t digit_decimal;
    uint8_t digit_0;
    uint8_t digit_1;
    uint8_t digit_2;
    bool is_minus;
    DecimalValue(int16_t _value) {
        value = _value;
        is_minus = value < 0;
        absolute_value = value & ~(1 << 15);
        uint16_t counter = absolute_value;
        digit_decimal = counter % 10;
        counter = (counter-digit_decimal) / 10;
        digit_0 = counter % 10;
        counter = (counter-digit_0) / 10;
        digit_1 = counter % 10;
        counter = (counter-digit_1) / 10;
        digit_2 = counter % 10;
    }
};

// temp*10 with 1 decimal precision
static void test_weather_page(int16_t temperature, uint16_t humidity) {
    RadialBackgroundColour radial;
    if (temperature < 0) {
        radial.background_colour = create_rgb565_u8(40, 40, 80);
        radial.delta_colour = create_rgb565_bits(2,4,2);
    } else if (temperature < 200) {
        radial.background_colour = create_rgb565_u8(30, 30, 80);
        radial.delta_colour = create_rgb565_bits(1,1,2);
    } else if (temperature < 300) {
        radial.background_colour = create_rgb565_u8(30, 80, 30);
        radial.delta_colour = create_rgb565_bits(1,4,1);
    } else {
        radial.background_colour = create_rgb565_u8(80, 30, 30);
        radial.delta_colour = create_rgb565_bits(2,1,1);
    }

    radial.x_start = 0;
    radial.y_start = 0;
    radial.x_end = tft::SCREEN_WIDTH-1;
    radial.y_end = tft::SCREEN_HEIGHT-1;
    radial.fill();

    const uint16_t x_margin = 10;
    const uint16_t y_margin = 10;
    uint16_t y_text_bottom = 0;

    const rgb565_t text_colour = COLOUR.WHITE;
    {
        namespace font = large_font;
        y_text_bottom += y_margin+font::MAX_HEIGHT;
        RightToLeftPrinter printer;
        printer.x_end = tft::SCREEN_WIDTH-1-x_margin;
        printer.y_end = y_text_bottom-1;
        printer.text_colour = text_colour;
        const auto decimal = DecimalValue(temperature);
        printer.print_char('C', radial, &font::get_glyph);
        printer.print_char(0xB0, radial, &font::get_glyph);
        printer.print_char('0'+decimal.digit_decimal, radial, &font::get_glyph);
        printer.print_char('.', radial, &font::get_glyph);
        printer.print_char('0'+decimal.digit_0, radial, &font::get_glyph);
        if (decimal.absolute_value >= 100) printer.print_char('0'+decimal.digit_1, radial, &font::get_glyph);
        if (decimal.is_minus) printer.print_char('-', radial, &font::get_glyph);
    }
    {
        namespace font = large_font;
        y_text_bottom += y_margin+font::MAX_HEIGHT;
        RightToLeftPrinter printer;
        printer.x_end = tft::SCREEN_WIDTH-1-x_margin;
        printer.y_end = y_text_bottom-1;
        printer.text_colour = text_colour;
        const auto decimal = DecimalValue(humidity);
        printer.print_char('%', radial, &font::get_glyph);
        printer.print_char('0'+decimal.digit_decimal, radial, &font::get_glyph);
        printer.print_char('.', radial, &font::get_glyph);
        printer.print_char('0'+decimal.digit_0, radial, &font::get_glyph);
        if (decimal.absolute_value >= 100) printer.print_char('0'+decimal.digit_1, radial, &font::get_glyph);
        if (decimal.absolute_value >= 1000) printer.print_char('0'+decimal.digit_2, radial, &font::get_glyph);
    }
    {
        namespace font = large_font;
        y_text_bottom += y_margin+font::MAX_HEIGHT;
        RightToLeftPrinter printer;
        printer.x_end = tft::SCREEN_WIDTH-1-x_margin;
        printer.y_end = y_text_bottom-1;
        printer.text_colour = text_colour;
        printer.print_string("02:27AM", radial, &font::get_glyph);
    }
    {
        namespace font = small_font;
        y_text_bottom += y_margin+font::MAX_HEIGHT;
        RightToLeftPrinter printer;
        printer.x_end = tft::SCREEN_WIDTH-1-x_margin;
        printer.y_end = y_text_bottom-1;
        printer.text_colour = text_colour;
        if (temperature < 0) {
            printer.print_string("HEAVY SNOWSTORM", radial, &font::get_glyph);
        } else if (temperature < 200) {
            printer.print_string("STRONG OVERCAST", radial, &font::get_glyph);
        } else if (temperature < 300) {
            printer.print_string("SUNNY", radial, &font::get_glyph);
        } else {
            printer.print_string("INTENSE HEATWAVE", radial, &font::get_glyph);
        }
    }
    {
        namespace font = small_font;
        y_text_bottom += y_margin+font::MAX_HEIGHT;
        LeftToRightPrinter printer;
        printer.x_start = x_margin;
        printer.y_end = y_text_bottom-1;
        printer.text_colour = text_colour;
        if (humidity < 100) {
            printer.print_string("DRY AIR", radial, &font::get_glyph);
        } else if (temperature < 300) {
            printer.print_string("MODERATE HUMIDITY", radial, &font::get_glyph);
        } else if (temperature < 600) {
            printer.print_string("HEATSTROKE RISK", radial, &font::get_glyph);
        } else {
            printer.print_string("UNDERWATER", radial, &font::get_glyph);
        }
    }
    {
        namespace font = small_font;
        y_text_bottom += y_margin+font::MAX_HEIGHT;
        LeftToRightPrinter printer;
        printer.x_start = x_margin;
        printer.y_end = y_text_bottom-1;
        printer.text_colour = text_colour;
        printer.print_string("WIND: 201.7KPH", radial, &font::get_glyph);
    }
    {
        namespace font = small_font;
        y_text_bottom += y_margin+font::MAX_HEIGHT;
        LeftToRightPrinter printer;
        printer.x_start = x_margin;
        printer.y_end = y_text_bottom-1;
        printer.text_colour = text_colour;
        printer.print_string("MOON: HALF MOON", radial, &font::get_glyph);
    }

    delay(1000);
    log_frame(std::format("Testing weather display with {0} degrees celcius", temperature));
}

static void app_setup() {
    Serial.begin(9600);
    Serial.println("Starting up st7789 controller");
    tft::init();
    tft::set_brightness(50);
}

static void app_loop() {
    // test_screen_colours();
    // test_tunnel_colours();
    // test_circle();
    // test_rgb_square();
    // test_glyphs_with_solid_background();
    // test_glyphs_with_radial_background();
    {
        test_weather_page(-110, 69);
        test_weather_page(95, 215);
        test_weather_page(217, 419);
        test_weather_page(328, 1000);
    }
}

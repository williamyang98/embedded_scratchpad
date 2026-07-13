#pragma once
#include "./response.hpp"
#include "./page.hpp"
#include "../utility/printers.hpp"
#include "../graphics/rgb565.hpp"
#include "../graphics/colour_functions.hpp"
#include "../hardware/tft.hpp"
#include "../hardware/pgmspace.h"
#include "../glyphs/small_font.hpp"

class LandingPage: public Page {
private:
    bool m_is_full_rerender = true;
    RadialBackgroundColour m_background_colour;
    LeftToRightPrinter m_top_printer;
    LeftToRightPrinter m_bottom_printer;
    uint16_t m_x_top_start;
    uint16_t m_x_bottom_start;
    uint16_t m_x_spacing = 10;
    rgb565_t m_text_colour = COLOUR.WHITE;
    const FlashMemory<char>* m_top_string;
    const FlashMemory<char>* m_bottom_string;
public:
    LandingPage() {
        m_top_printer.text_colour = m_text_colour;
        m_bottom_printer.text_colour = m_text_colour;
        m_background_colour.background_colour = create_rgb565_bits(4, 8, 4);
        m_background_colour.delta_colour = create_rgb565_bits(1,2,1);

        namespace font = small_font;
        m_top_printer.y_end = tft::SCREEN_HEIGHT/2 - m_x_spacing/2;
        m_bottom_printer.y_end = tft::SCREEN_HEIGHT/2 + m_x_spacing/2 + font::MAX_HEIGHT;

        m_top_string = FLASH_STRING("WAITING FOR");
        m_bottom_string = FLASH_STRING("CONNECTION");

        m_x_top_start = tft::SCREEN_WIDTH/2 - calculate_text_width(m_top_string, font::get_glyph)/2;
        m_x_bottom_start = tft::SCREEN_WIDTH/2 - calculate_text_width(m_bottom_string, font::get_glyph)/2;
    }
    void mark_for_full_rerender() {
        m_is_full_rerender = true;
    }
    void render_all() {
        g_response_sender.send_render_status(true);
        if (m_is_full_rerender) {
            render_background();
            render_text();
            m_is_full_rerender = false;
            DEBUG_FRAME("Rendering landing page");
        }
        g_response_sender.send_render_status(false);
    }
private:
    void render_background() {
        m_background_colour.x_start = 0;
        m_background_colour.y_start = 0;
        m_background_colour.x_end = tft::SCREEN_WIDTH-1;
        m_background_colour.y_end = tft::SCREEN_HEIGHT-1;
        m_background_colour.fill();
    }
    void render_text() {
        namespace font = small_font;
        m_top_printer.x_start = m_x_top_start;
        m_bottom_printer.x_start = m_x_bottom_start;

        m_top_printer.print_string(m_top_string, m_background_colour, font::get_glyph);
        m_bottom_printer.print_string(m_bottom_string, m_background_colour, font::get_glyph);
        m_top_printer.cleanup_previous_prints(font::MAX_HEIGHT, m_background_colour);
        m_bottom_printer.cleanup_previous_prints(font::MAX_HEIGHT, m_background_colour);
    }
};

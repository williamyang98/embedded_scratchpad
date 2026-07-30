#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "./hardware/tft.hpp"
#include "./hardware/response_output.hpp"
#include "./graphics/render_glyphs.hpp"
#include "./app/app.hpp"
#include "./app/commands.hpp"
#include "./app/response.hpp"
#include "./st7789.hpp"
#include "./cobs_decoder.hpp"

ST7789 g_st7789(tft::SCREEN_WIDTH, tft::SCREEN_HEIGHT); // extern

#if _WIN32
#define NOMINMAX
#include <io.h>
#include <fcntl.h>
#endif

class FileInput {
private:
    const FILE* m_fp_in;
public:
    FileInput(FILE* fp_in): m_fp_in(fp_in) {
        #if _WIN32
        _setmode(_fileno(m_fp_in), _O_BINARY);
        #endif
    }
    ~FileInput() {
        fclose(m_fp_in);
    }
    int read() {
        uint8_t data = 0;
        const size_t total_read = fread(&data, sizeof(uint8_t), 1, m_fp_in);
        if (total_read == 0) return -1;
        return static_cast<int>(data);
    }
};

int main(int argc, char** argv) {
    ResponseOutput response_output;
    ResponseSender response_sender(response_output);
    App app(response_sender);

    CommandParser command_parser(response_sender, app);
    CobsDecoder cobs_decoder(command_parser);
    FileInput file_input(stdin);

    response_sender.send_message("Initating ST7789 sketch");
    tft::init();
    tft::set_brightness(50);
    tft::set_write_mode(false, false);
    g_glyph_rgba_q256_palette_render_settings.x_mirror = false;
    g_glyph_rgba_q256_palette_render_settings.y_mirror = false;
    app.render_all();
    app.set_page(AppPage::WEATHER_PAGE);

    while (true) {
        const int result = file_input.read();
        if (result == -1) break;
        const uint8_t data = static_cast<uint8_t>(result);
        cobs_decoder.handle_incoming_byte(data);
    }
    return 0;
}

#include <stdio.h>
#include <stdint.h>
#include "./app.hpp"
#include "./st7789.hpp"

#if _WIN32
#define NOMINMAX
#include <io.h>
#include <fcntl.h>
#endif

FILE* fp_in; // extern
FILE* fp_out; // extern
std::unique_ptr<ST7789> st7789 = nullptr; // extern

int main(int argc, char** argv) {
    fp_in = stdin;
    fp_out = stdout;

#if _WIN32
    _setmode(_fileno(fp_in), _O_BINARY);
    _setmode(_fileno(fp_out), _O_BINARY);
#endif

    app_setup();
    while (true) {
        app_loop();
        break;
    }
    return 0;
}

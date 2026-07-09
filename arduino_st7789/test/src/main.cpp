#include <stdint.h>
#include <thread>
#include "./sketch.hpp"

HardwareSerial Serial; // extern
ST7789 st7789(tft::SCREEN_WIDTH, tft::SCREEN_HEIGHT); // extern
App app; // extern
CommandParser command_parser(app); // extern

int main(int argc, char** argv) {
    sketch_setup();

    CommandSender command_sender(Serial.m_pipe_in);
    auto thread_stdin = std::thread([&command_sender]() {
        command_sender.set_temperature(100);
        command_sender.set_humidity(200);
        command_sender.set_time_24_hour(1345, false, false);
        command_sender.set_wind_kph(35);
        command_sender.trigger_render();
        command_sender.close();
    });

    while (true) {
        sketch_loop();
        break;
    }
    thread_stdin.join();

    return 0;
}

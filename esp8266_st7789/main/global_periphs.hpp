#ifndef __GLOBAL_PERIPHERALS_H__
#define __GLOBAL_PERIPHERALS_H__

#include <driver/gpio.h>
#include "dht11.hpp"
#include "websocket.h"
#include "hardware/response_output.hpp"
#include "app/response.hpp"
#include "app/app.hpp"
#include "app/commands.hpp"
#include "extra_led.hpp"
#include "extra_button.hpp"

extern httpd_handle_t g_http_server;
extern DHT11 g_dht11;
extern struct Websocket g_websocket;
extern ResponseOutput g_response_output;
extern ResponseSender g_response_sender;
extern App g_app;
extern CommandParser g_command_parser;
extern ExtraButton g_flash_button;
extern ExtraLed g_green_led;

#endif

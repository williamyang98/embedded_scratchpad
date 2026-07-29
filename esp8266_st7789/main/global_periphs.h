#ifndef __GLOBAL_PERIPHERALS_H__
#define __GLOBAL_PERIPHERALS_H__

#include <driver/gpio.h>
#include "dht11.h"
#include "websocket.h"

extern const gpio_num_t g_dht11_data_pin;
extern struct Websocket g_websocket;

#endif

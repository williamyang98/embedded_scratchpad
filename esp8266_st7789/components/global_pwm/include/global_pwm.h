#pragma once

#include <esp_err.h>
#include <driver/gpio.h>
#include <stdint.h>

#define GLOBAL_PWM_MAX_CHANNELS 8
#define GLOBAL_PWM_PERIOD_US 1024

// helper to globally register application pwm channels
// then afterwards just use regular pwm functions from esp8266-rtos-sdk
// https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/api-reference/peripherals/pwm.html
esp_err_t global_pwm_add_channel(gpio_num_t pin, uint8_t* channel);
esp_err_t global_pwm_init(void);
const uint32_t* global_pwm_get_pins(void);
uint8_t global_pwm_get_total_pins(void);


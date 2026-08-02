#include "global_pwm.h"

#include <assert.h>
#include <esp_log.h>
#include <esp_err.h>
#include <stdint.h>
#include <stdbool.h>
#include <driver/pwm.h>

static uint32_t pwm_duty_cycles[GLOBAL_PWM_MAX_CHANNELS] = {0};
static float pwm_phases[GLOBAL_PWM_MAX_CHANNELS] = {0.0f};
static uint32_t pin_numbers[GLOBAL_PWM_MAX_CHANNELS] = {0};
static uint8_t total_registered_pins = 0;
static bool is_initialised = false;

static const char TAG[] = "global_pwm";

esp_err_t global_pwm_add_channel(gpio_num_t pin, uint8_t* channel) {
    assert(channel != NULL);
    const uint8_t new_channel = total_registered_pins;
    if (new_channel >= GLOBAL_PWM_MAX_CHANNELS) {
        ESP_LOGE(TAG, "Tried to register more than %u global pwm channels when adding pin %u for channel %u", GLOBAL_PWM_MAX_CHANNELS, pin, new_channel);
        return ESP_FAIL;
    }
    pin_numbers[new_channel] = (uint32_t)pin;
    pwm_duty_cycles[new_channel] = 0;
    pwm_phases[new_channel] = 0;
    total_registered_pins++;
    *channel = new_channel;

    ESP_LOGI(TAG, "Registered pin %u on global pwm channel %u", pin, new_channel);
    return ESP_OK;
}

esp_err_t global_pwm_init(void) {
    if (is_initialised) {
        ESP_LOGE(TAG, "Tried to initialise again!");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Attempting to initialise %u global pwm channels", total_registered_pins);
    if (total_registered_pins != 0) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(pwm_init(GLOBAL_PWM_PERIOD_US, pwm_duty_cycles, total_registered_pins, pin_numbers));
        ESP_ERROR_CHECK_WITHOUT_ABORT(pwm_set_phases(pwm_phases));
        ESP_ERROR_CHECK_WITHOUT_ABORT(pwm_start());
    }
    is_initialised = true;
    return ESP_OK;
}

const uint32_t* global_pwm_get_pins(void) {
    return pin_numbers;
}

uint8_t global_pwm_get_total_pins(void) {
    return total_registered_pins;
}

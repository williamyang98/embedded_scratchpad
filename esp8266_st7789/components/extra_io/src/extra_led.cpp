#include "extra_led.hpp"
#include <assert.h>
#include <esp_err.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include <driver/pwm.h>
extern "C" {
#include "global_pwm.h"
}

static const char TAG[] = "extra-led";

void ExtraLed::init() {
    assert(!m_is_init);
    m_is_init = true;
    ESP_ERROR_CHECK(gpio_set_direction(m_pin, GPIO_MODE_OUTPUT));
    ESP_LOGI(TAG, "set gpio direction for pin=%u, label=%s", m_pin, m_label);
    ESP_ERROR_CHECK(global_pwm_add_channel(m_pin, &m_pwm_channel));
    ESP_LOGI(TAG, "registered global pwm channel for pin=%u, channel=%u, label=%s", m_pin, m_pwm_channel, m_label);
}

void ExtraLed::set_duty_cycle(uint32_t duty_cycle) {
    assert(m_is_init);
    ESP_ERROR_CHECK(pwm_set_duty(m_pwm_channel, duty_cycle));
    ESP_ERROR_CHECK(pwm_start());
}

uint32_t ExtraLed::get_duty_cycle() const {
    assert(m_is_init);
    uint32_t duty_cycle = 0;
    ESP_ERROR_CHECK(pwm_get_duty(m_pwm_channel, &duty_cycle));
    return duty_cycle;
}

#pragma once

#include <stdint.h>
#include <driver/gpio.h>

class ExtraLed {
private:
    gpio_num_t m_pin;
    const char* m_label;
    uint8_t m_pwm_channel = 0;
    bool m_is_init = false;
public:
    ExtraLed(gpio_num_t pin, const char* label) {
        m_pin = pin;
        m_label = label;
    }
    void init();
    void set_duty_cycle(uint32_t duty_cycle);
    uint32_t get_duty_cycle() const; 
};

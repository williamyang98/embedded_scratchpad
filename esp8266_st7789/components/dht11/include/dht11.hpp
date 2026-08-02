#pragma once

#include <esp_err.h>
#include <driver/gpio.h>

class DHT11 {
public:
    struct Measurement {
        uint8_t temperature;
        uint8_t humidity;
    };
    enum class ReadStatus: uint8_t {
        OK = 0x00,
        START_PULLDOWN_1_TIMEOUT = 0x01,
        START_PULLUP_TIMEOUT = 0x01,
        START_PULLDOWN_2_TIMEOUT = 0x02,
        DATA_START_TO_TRANSMIT_TIMEOUT = 0x3,
        DATA_PULLDOWN_TIMEOUT = 0x4,
        DATA_PULLUP_TIMEOUT = 0x5,
        DATA_PULLUP_TOO_SHORT = 0x6,
        DATA_PULLUP_TOO_LONG = 0x7,
        CHECKSUM_FAIL = 0x8,
    };
private:
    gpio_num_t m_pin;
    bool m_is_init = false;
public:
    DHT11(gpio_num_t pin) {
        m_pin = pin;
    }
    void init();
    ReadStatus read(struct Measurement& measurement);
private:
    ReadStatus read_data(struct Measurement& measurement);
    int32_t wait_signal_us(uint32_t timeout_us, uint32_t level);
};

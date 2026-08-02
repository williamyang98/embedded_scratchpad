#include "dht11.hpp"

#include <assert.h>
#include <esp_err.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char TAG[] = "dht11";
static constexpr int TOTAL_DATA_LENGTH = 5; // 4 data and 1 checksum

// DOC: datasheets/dht11_datasheet.pdf
void DHT11::init() {
    assert(!m_is_init);
    m_is_init = true;
    // dht11 uses single wire communication that is connected to a pull-up resistor
    // outputing 0 to the line means pulling it down (open drain), and 1 means leaving it pulled up
    gpio_config_t config = {
        .pin_bit_mask = (1u << m_pin),
        .mode = GPIO_MODE_OUTPUT_OD,
        // NOTE: DHT11 breakout board comes with its own pull up resistor to Vcc
        // This might be connected to 5V instead of the ESP8266's 3.3V
        // So adding another pull up resistor would set the overall pull up voltage to something in the middle
        // .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&config));
    ESP_LOGI(TAG, "initialised dht11 sensor on pin=%u", m_pin);
}

DHT11::ReadStatus DHT11::read(struct DHT11::Measurement& measurement) {
    assert(m_is_init);

    // pulldown for at least 18ms
    ESP_ERROR_CHECK(gpio_set_level(m_pin, 0));
    vTaskDelay(30/portTICK_PERIOD_MS);
    /// so we can use os_delay without caused a core meditation error and disable interrupts
    taskENTER_CRITICAL();
    ESP_ERROR_CHECK(gpio_set_level(m_pin, 1)); // immediately stop pulling down so dht11 can send out response
    const auto status = read_data(measurement);
    taskEXIT_CRITICAL();
    if (status != ReadStatus::OK) {
        ESP_LOGE(TAG, "failed to read data");
    }
    return status;
}

DHT11::ReadStatus DHT11::read_data(struct DHT11::Measurement& measurement) {
    constexpr uint32_t START_PULLDOWN_1_TIMEOUT_US = 60;
    constexpr uint32_t START_PULLUP_TIMEOUT_US = 100;
    constexpr uint32_t START_PULLDOWN_2_TIMEOUT_US = 100;
    // wait for pull down response after 20 to 40 us
    if (wait_signal_us(START_PULLDOWN_1_TIMEOUT_US, 0) < 0) {
        ESP_LOGE(TAG, "start timeout on pull down #1 after %uus", START_PULLDOWN_1_TIMEOUT_US);
        return ReadStatus::START_PULLDOWN_1_TIMEOUT;
    }
    // wait for pull up after 80us
    if (wait_signal_us(START_PULLUP_TIMEOUT_US, 1) < 0) {
        ESP_LOGE(TAG, "start timeout on pull up after %uus", START_PULLUP_TIMEOUT_US);
        return ReadStatus::START_PULLUP_TIMEOUT;
    }
    // pulls down after 80us
    if (wait_signal_us(START_PULLDOWN_2_TIMEOUT_US, 0) < 0) {
        ESP_LOGE(TAG, "start timeout on pull down #2 after %uus", START_PULLDOWN_2_TIMEOUT_US);
        return ReadStatus::START_PULLDOWN_2_TIMEOUT;
    }

    // DHT11 data signal
    // Each bit starts with 50us low voltage
    // and ends with a high voltage
    // 26-28us means 0
    // 70us means 1
    constexpr uint32_t DATA_START_TO_TRANSMIT_TIMEOUT_US = 70;
    constexpr uint32_t DATA_PULLUP_TIMEOUT_US = 80;
    constexpr uint32_t MIN_DATA_PULLUP_DURATION_US = 4;
    constexpr uint32_t MAX_DATA_PULLUP_DURATION_US = 80;
    constexpr uint32_t DATA_LOW_DURATION_US = 30;

    uint8_t buffer[TOTAL_DATA_LENGTH] = {0};
    for (int current_byte = 0; current_byte < TOTAL_DATA_LENGTH; current_byte++) {
        buffer[current_byte] = 0x00;
        for (int current_bit = 0; current_bit < 8; current_bit++) {
            // 50us pulldown
            if (wait_signal_us(DATA_START_TO_TRANSMIT_TIMEOUT_US, 1) < 0) {
                ESP_LOGE(TAG, "timeout pulldown on byte %d bit %d after %uus",
                    current_byte, current_bit, DATA_START_TO_TRANSMIT_TIMEOUT_US);
                return ReadStatus::DATA_START_TO_TRANSMIT_TIMEOUT;
            }
            // read pull up length
            const int32_t duration = wait_signal_us(DATA_PULLUP_TIMEOUT_US, 0);
            if (duration < 0) {
                ESP_LOGE(TAG, "timeout pullup on byte %d bit %d after %uus",
                    current_byte, current_bit, DATA_PULLUP_TIMEOUT_US);
                return ReadStatus::DATA_PULLUP_TIMEOUT;
            }

            if (duration < MIN_DATA_PULLUP_DURATION_US) {
                ESP_LOGE(TAG, "pullup too short on byte %d bit %d. duration=%dus < %uus",
                    current_byte, current_bit, duration, MIN_DATA_PULLUP_DURATION_US);
                return ReadStatus::DATA_PULLUP_TOO_SHORT;
            }

            if (duration > MAX_DATA_PULLUP_DURATION_US) {
                ESP_LOGE(TAG, "pullup too long on byte %d bit %d. duration=%dus > %uus",
                    current_byte, current_bit, duration, MAX_DATA_PULLUP_DURATION_US);
                return ReadStatus::DATA_PULLUP_TOO_LONG;
            }

            if (duration <= DATA_LOW_DURATION_US) {
                // Byte is already zeroed out at start
                // buffer[current_byte] &= ~(1 << (7-current_bit));
            } else {
                buffer[current_byte] |= (1 << (7-current_bit));
            }
        }
    }

    // confirm checksum
    const uint8_t checksum = (buffer[0] + buffer[1] + buffer[2] + buffer[3]) & 0xFF;
    if (checksum != buffer[4]) {
        ESP_LOGE(TAG, "failed checksum 0x%x != 0x%x, calculated != expected", checksum, buffer[4]);
        ESP_LOGE(TAG, "buffer contents are: %d, %d, %d, %d, %d", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]);
        return ReadStatus::CHECKSUM_FAIL;
    }

    measurement.temperature = buffer[2];
    measurement.humidity = buffer[0];
    return ReadStatus::OK;
}

int32_t DHT11::wait_signal_us(uint32_t timeout_us, uint32_t level) {
    constexpr uint32_t tick_us = 2; // no 1us timings
    for (uint32_t i = 0; i < timeout_us; i+=tick_us) {
        if (gpio_get_level(m_pin) == level) {
            return static_cast<int32_t>(i);
        }
        os_delay_us(tick_us);
    }
    return -1;
}

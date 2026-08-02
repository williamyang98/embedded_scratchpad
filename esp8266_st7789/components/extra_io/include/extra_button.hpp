#pragma once

#include <esp_err.h>
#include <driver/gpio.h>
#include <vector>

typedef void* TaskHandle_t;
typedef void* QueueHandle_t;

class ExtraButton {
public:
    typedef void (*Callback)(bool is_pressed, void* args);
private:
    struct Handler {
        Callback callback;
        void* args;
        bool operator==(const Handler& other) const {
            return callback == other.callback && args == other.args;
        }
        bool operator!=(const Handler& other) const {
            return !(*this == other);
        }
    };
    gpio_num_t m_pin;
    int m_pin_func; // FUNC_GPIOx
    const char* m_label;
    QueueHandle_t m_rtos_queue = nullptr;
    TaskHandle_t m_rtos_task = nullptr;
    std::vector<Handler> m_handlers;
    bool m_is_init = false;
public:
    ExtraButton(gpio_num_t pin, int pin_func, const char* label) {
        m_pin = pin;
        m_pin_func = pin_func;
        m_label = label;
    }
    void init();
    bool is_pressed() const;
    esp_err_t attach_callback(Callback callback, void* args, bool is_immediate);
    esp_err_t remove_callback(Callback callback, void* args);
private:
    static void receive_pressed(void* _this);
    static void isr_enqueue_pressed(void* _this);
};

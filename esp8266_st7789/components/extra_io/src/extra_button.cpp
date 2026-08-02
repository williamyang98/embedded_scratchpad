#include "extra_button.hpp"
#include <assert.h>
#include <esp_err.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include <FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <freertos/queue.h>

static const char TAG[] = "extra-button";
static constexpr int BUTTON_QUEUE_LENGTH = 8;
static constexpr int BUTTON_TASK_PRIORITY = 10;
static constexpr int BUTTON_TASK_STACK_SIZE = 1024;

void ExtraButton::init() {
    assert(!m_is_init);
    m_is_init = true;
    PIN_FUNC_SELECT(PERIPHS_GPIO_MUX_REG(m_pin), m_pin_func);
    ESP_LOGI(TAG, "setup pin function for pin=%u, label=%s", m_pin, m_label);

    gpio_config_t config = {
        .pin_bit_mask = (1u << m_pin),
        .mode = GPIO_MODE_INPUT,
        // button shorts to ground
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&config));
    ESP_LOGI(TAG, "setup gpio config for pin=%u, label=%s", m_pin, m_label);

    // interrupt_service_routine -> freertos_async_queue -> freertos_task_queue -> handlers
    // freertos button press/release task queue
    m_rtos_queue = xQueueCreate(BUTTON_QUEUE_LENGTH, sizeof(bool));
    assert(m_rtos_queue != NULL);
    assert(xTaskCreate(
        ExtraButton::receive_pressed,
        m_label,
        BUTTON_TASK_STACK_SIZE,
        (void*)this,
        BUTTON_TASK_PRIORITY,
        &m_rtos_task
    ) == pdPASS);
    ESP_LOGI(TAG, "created freertos queue and task for pin=%u, label=%s", m_pin, m_label);

    // requires gpio_install_isr_service() first
    ESP_ERROR_CHECK(gpio_isr_handler_add(m_pin, ExtraButton::isr_enqueue_pressed, (void*)this));
    ESP_LOGI(TAG, "added isr handler for pin=%u, label=%s", m_pin, m_label);
}

bool ExtraButton::is_pressed() const {
    assert(m_is_init);
    // on press it is pulled down
    return !gpio_get_level(m_pin);
}

esp_err_t ExtraButton::attach_callback(Callback callback, void* args, bool is_immediate) {
    assert(m_is_init);
    assert(callback != nullptr);
    const Handler new_handler = {
        .callback = callback,
        .args = args,
    };
    for (const auto& handler: m_handlers) {
        if (handler == new_handler) {
            ESP_LOGE(TAG, "Tried to add %s handler twice to a list of %u %s handlers", m_label, m_handlers.size(), m_label);
            return ESP_FAIL;
        }
    }
    m_handlers.push_back(new_handler);
    ESP_LOGI(TAG, "Added %s handler for a total of %u %s handlers afterwards", m_label, m_handlers.size(), m_label);
    if (is_immediate) {
        ESP_LOGI(TAG, "Immediately firing %s handler", m_label);
        callback(is_pressed(), args);
    }
    return ESP_OK;
}

esp_err_t ExtraButton::remove_callback(Callback callback, void* args) {
    assert(m_is_init);
    assert(callback != nullptr);
    const Handler handler = {
        .callback = callback,
        .args = args,
    };
    const size_t total_handlers = m_handlers.size();
    for (size_t i = 0; i < total_handlers; i++) {
        const auto& old_handler = m_handlers[i];
        if (old_handler != handler) continue;
        // replace with last item
        m_handlers[i] = std::move(m_handlers.back());
        m_handlers.pop_back();
        ESP_LOGI(TAG, "Remove %s handler for a total of %u %s handlers remaining", m_label, m_handlers.size(), m_label);
        return ESP_OK;
    }
    ESP_LOGE(TAG, "Tried to remove an untracked %s handler from a list of %u %s handlers", m_label, m_handlers.size(), m_label);
    return ESP_FAIL;
}

void ExtraButton::receive_pressed(void* _this) {
    assert(_this != NULL);
    ExtraButton* self = reinterpret_cast<ExtraButton*>(_this);
    assert(self->m_is_init);
    bool prev_is_pressed = self->is_pressed();
    while (true) {
        bool is_pressed = false;
        if (!xQueueReceive(self->m_rtos_queue, &is_pressed, portMAX_DELAY)) continue;
        const bool is_changed = prev_is_pressed != is_pressed;
        prev_is_pressed = is_pressed;
        // notify all listeners
        if (is_changed) {
            for (const auto& handler: self->m_handlers) {
                handler.callback(is_pressed, handler.args);
            }
        }
    }
}

void IRAM_ATTR ExtraButton::isr_enqueue_pressed(void* _this) {
    assert(_this != NULL);
    ExtraButton* self = reinterpret_cast<ExtraButton*>(_this);
    const bool is_pressed = self->is_pressed();
    const auto status = xQueueSendFromISR(self->m_rtos_queue, &is_pressed, NULL);
    if (status == errQUEUE_FULL) {
        ESP_LOGE(TAG, "too many button pressed for pin=%u, label=%s, is_pressed=%u", self->m_pin, self->m_label, is_pressed);
    }
}


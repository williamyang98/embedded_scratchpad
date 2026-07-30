extern "C" {
#include "dht11.h"
}

#include "websocket_handler.hpp"
#include "global_periphs.hpp"
#include <memory>
#include <vector>
#include <esp_log.h>
#include <esp_err.h>

static const char TAG[] = "websocket-handler";
// Refer to CommandHeader in components/st7789/app/commands.hpp to determine what headers are already taken
static const uint8_t DHT11_CMD = 0x0A;

static struct WebsocketClient* dht11_websocket_client = NULL;

static void websocket_async_send_dht11(struct WebsocketClient* client, void *args) {
    static const char SUBTAG[] = "dht11-async-websocket-handler";
    assert(client != NULL);
    const struct Websocket* websocket = client->websocket;
    assert(websocket != NULL);
    uint8_t* buffer = websocket->transmit_buffer;
    assert(buffer != NULL);

    size_t length = 0;
    buffer[0] = DHT11_CMD;
    struct DHT11_Measurement measurement;
    const bool is_read_success = dht11_read(g_dht11_data_pin, &measurement) == ESP_OK;
    if (is_read_success) {
        buffer[1] = measurement.humidity;
        buffer[2] = measurement.temperature;
        length = 3;
    } else {
        buffer[1] = 0xFF;
        length = 2;
    }
    const esp_err_t status = websocket_send_pending_binary_data_async(client, length);
    if (status != ESP_OK) {
        ESP_LOGE(SUBTAG, "Failed to send dht11 data websocket_fd=%d, humidity=%u, temperature=%u, error='%s'",
            client->websocket_fd, measurement.humidity, measurement.temperature, esp_err_to_name(status)
        );
    }
}

static void websocket_on_dht11_frame(httpd_req_t* request, struct WebsocketClient* client, const uint8_t* data, size_t size) {
    static const char SUBTAG[] = "dht11-websocket-handler";
    assert(request != NULL);
    assert(client != NULL);
    dht11_websocket_client = client;
    const esp_err_t status = websocket_queue_async_task(client, websocket_async_send_dht11, NULL);
    if (status != ESP_OK) {
        ESP_LOGE(SUBTAG, "failed to queue async dht11 task: '%s'", esp_err_to_name(status));
        dht11_websocket_client = NULL;
    }
}

static void websocket_async_send_app_response(struct WebsocketClient* client, void* _app_response) {
    static const char SUBTAG[] = "response-output-async-websocket-handler";
    assert(client != NULL);
    const struct Websocket* websocket = client->websocket;
    assert(websocket != NULL);

    assert(_app_response != NULL);
    auto app_response = std::unique_ptr<std::vector<uint8_t>>(static_cast<std::vector<uint8_t>*>(_app_response));

    const uint8_t* src_buffer = app_response->data();
    const size_t src_size = app_response->size();
    uint8_t* dest_buffer = websocket->transmit_buffer;
    assert(dest_buffer != NULL);
    assert(src_buffer != NULL);
    assert(websocket->transmit_buffer_size >= src_size);
    memcpy(dest_buffer, src_buffer, src_size);

    const esp_err_t status = websocket_send_pending_binary_data_async(client, src_size);
    if (status != ESP_OK) {
        ESP_LOGE(SUBTAG, "Failed to send app response websocket_fd=%d, buffer_size=%u, error='%s'",
            client->websocket_fd, src_size, esp_err_to_name(status)
        );
    }
}

static void on_app_response_callback(const uint8_t* buffer, size_t size, void* _client) {
    static const char SUBTAG[] = "response-output-callback";
    struct WebsocketClient* client = (struct WebsocketClient*)_client;
    assert(client != NULL);

    // need to copy this since websocket_queue_async may take some time
    // which can result in the scratch buffer used by ResponseOutput to be overwritten with another response
    auto app_response = std::unique_ptr<std::vector<uint8_t>>(new std::vector<uint8_t>);
    app_response->resize(size);
    memcpy(app_response->data(), buffer, size);

    const esp_err_t status = websocket_queue_async_task(client, websocket_async_send_app_response, app_response.release());
    if (status != ESP_OK) {
        ESP_LOGE(SUBTAG, "failed to queue async app response: websocket_fd=%d, buffer_size=%u, error='%s'",
            client->websocket_fd, size, esp_err_to_name(status)
        );
    }
}

static void websocket_on_open(httpd_req_t* request, struct WebsocketClient* client) {
    g_response_output.attach_callback(on_app_response_callback, (void*)client);
}

static void websocket_on_binary_frame(httpd_req_t* request, struct WebsocketClient* client, const uint8_t* data, size_t size) {
    static const char SUBTAG[] = "binary-frame-dispatcher-websocket-handler";
    assert(client != NULL);
    assert(data != NULL);

    if (size == 0) {
        ESP_LOGE(SUBTAG, "Got an unexpected empty command buffer");
        return;
    }

    const uint8_t cmd_code = data[0];
    const uint8_t *cmd_data = &data[1];
    int cmd_length = size-1;

    switch (cmd_code) {
    case DHT11_CMD: return websocket_on_dht11_frame(request, client, cmd_data, cmd_length);
    default: g_command_parser.parse_command(data, size); // redirect to app command handler
    }
}

// client will be freed after this call
static void websocket_on_close(httpd_req_t* request, struct WebsocketClient* client) {
    g_response_output.remove_callback(on_app_response_callback, (void*)client);
}

void websocket_attach_handlers(struct Websocket* websocket) {
    assert(websocket != NULL);
    websocket->on_open = websocket_on_open;
    websocket->on_binary_frame = websocket_on_binary_frame;
    websocket->on_close = websocket_on_close;
}
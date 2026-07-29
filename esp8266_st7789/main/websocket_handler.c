#include "websocket_handler.h"
#include "global_periphs.h"
#include "dht11.h"
#include <esp_log.h>

static const char TAG[] = "websocket-handler";
static const uint8_t DHT11_CMD = 0x03;

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

static void websocket_on_open(httpd_req_t* request, struct WebsocketClient* client) {
    // pc_io_status_listen(&g_pc_io_config, pc_io_status_listener, (void*)client);
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
    case DHT11_CMD: websocket_on_dht11_frame(request, client, cmd_data, cmd_length); break;
    default:        ESP_LOGD(TAG, "Unknown cmd: 0x%02x", cmd_code); break;
    }
}

// client will be freed after this call
static void websocket_on_close(httpd_req_t* request, struct WebsocketClient* client) {
    // pc_io_status_unlisten(&g_pc_io_config, pc_io_status_listener, (void*)client);
}

void websocket_attach_handlers(struct Websocket* websocket) {
    assert(websocket != NULL);
    websocket->on_open = websocket_on_open;
    websocket->on_binary_frame = websocket_on_binary_frame;
    websocket->on_close = websocket_on_close;
}
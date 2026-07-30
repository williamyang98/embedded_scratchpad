/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

extern "C" {
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <rom/ets_sys.h>
#include <nvs_flash.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_spi_flash.h>
#include <esp_spiffs.h>
#include <esp_system.h>
#include "dht11.h"
#include "global_periphs.h"
#include "webserver.h"
#include "websocket.h"
#include "websocket_handler.h"
#include "wifi_sta.h"
}

#include "hardware/tft.hpp"
#include "hardware/response_output.hpp"
#include "graphics/render_glyphs.hpp"
#include "app/response.hpp"
#include "app/app.hpp"

const char INIT_TAG[] = "main-init";

const gpio_num_t g_dht11_data_pin = GPIO_NUM_2; // extern
struct Websocket g_websocket = { // extern
    // buffers
    .receive_buffer_size = 0,
    .transmit_buffer_size = 0,
    .receive_buffer = NULL,
    .transmit_buffer = NULL,
    // handles
    .uri = "/api/v1/websocket",
    .server = NULL,
    .clients = NULL,
    // callbacks
    .on_binary_frame = NULL,
    .on_open = NULL,
    .on_close = NULL,
};

static httpd_handle_t http_server = NULL;

static esp_err_t init_nvs(void);
static esp_err_t init_server(void);

extern "C" void app_main(void) {
    ESP_LOGI(INIT_TAG, "entering main function!");
    esp_set_cpu_freq(ESP_CPU_FREQ_160M);
    ESP_LOGI(INIT_TAG, "Changing cpu to 160MHz");

    if (dht11_init(g_dht11_data_pin) == ESP_OK) {
        ESP_LOGI(INIT_TAG, "initialised dht11 sensor on pin: %u", g_dht11_data_pin);
    } else {
        ESP_LOGE(INIT_TAG, "failed to initialise dht11 sensor on pin: %u", g_dht11_data_pin);
    }

    tft::init();
    init_nvs();
    wifi_init_sta();
    init_server();

    // LINK: https://esp32.com/viewtopic.php?p=6023&sid=48c7254ec4cbe0d99f743e1d3687894d#p6023
    //       vTaskStartScheduler() is already called before app_main() so don't call it again
    // vTaskStartScheduler();
    // ESP_LOGI(INIT_TAG, "starting task scheduler!");

    ESP_LOGI(INIT_TAG, "finished initialisation");

    ResponseOutput response_output;
    ResponseSender response_sender(response_output);
    App app(response_sender);
    tft::set_brightness(50);
    tft::set_write_mode(false, false);
    g_glyph_rgba_q256_palette_render_settings.x_mirror = false;
    g_glyph_rgba_q256_palette_render_settings.y_mirror = false;
    app.render_all();
    app.set_page(AppPage::WEATHER_PAGE);
    app.render_all();
}

esp_err_t init_server(void) {
    // startup webserver
    const uint16_t port = 80;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;
    config.max_uri_handlers = 16; // we are serving many static files

    const esp_err_t start_status = httpd_start(&http_server, &config);
    if (start_status == ESP_OK) {
        ESP_LOGI(INIT_TAG, "created http server on port=%d", port);
    } else {
        ESP_LOGE(INIT_TAG, "failed to start webserver on port=%d (%s)", port, esp_err_to_name(start_status));
        return ESP_FAIL;
    }
    
    const esp_err_t register_status = webserver_register_endpoints(http_server);
    if (register_status == ESP_OK) {
        ESP_LOGI(INIT_TAG, "registered webserver endpoints on port=%d", port);
    } else {
        ESP_LOGE(INIT_TAG, "failed to register endpoints on port=%d (%s)", port, esp_err_to_name(register_status));
        return ESP_FAIL;
    }

    const esp_err_t websocket_register_status = websocket_register(http_server, &g_websocket, 64);
    if (websocket_register_status == ESP_OK) {
        ESP_LOGI(INIT_TAG, "registered websocket handler on port=%d", port);
        websocket_attach_handlers(&g_websocket);
    } else {
        ESP_LOGE(INIT_TAG, "failed to register websocket handler on port=%d, err='%s'", port, esp_err_to_name(websocket_register_status));
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t init_nvs(void) {
    const esp_err_t nvs_status = nvs_flash_init();
    if (nvs_status == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_LOGI(INIT_TAG, "no free NVS pages, erasing and reinitialising");
        nvs_flash_erase();
        nvs_flash_init();
    }
    ESP_LOGI(INIT_TAG, "starting NVS!");
    return ESP_OK;
}

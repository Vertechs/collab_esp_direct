#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "driver/gpio.h"
#include "nvs_flash.h"

#include "esp_event.h"
#include "esp_log.h"

#include "wifi_setup.h"

#include "tcp_servers.h"

#include "cfg_dispatcher.h"

#include "stepper_config.h"
#include "stepper_driver.h"

// Define Network Settings
#define WIFI_SSID      "Hello There"
#define WIFI_PASSWORD  "FishTreeCatLamp"
#define WIFI_MAX_RETRY  5

// Define Status LED
#define STATUS_GPIO 2
#define MAIN_PERIOD 1000

// logger name
static const char *TAG = "Main";


// setup status LED pin and timer?
static void status_setup(void)
{
    gpio_reset_pin(STATUS_GPIO);
    gpio_set_direction(STATUS_GPIO, GPIO_MODE_OUTPUT);
}

void app_main(void)
{
    // Init NVS for wifi driver
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    // Connect to WiFi
    if (networking_wifi_connect(WIFI_SSID, WIFI_PASSWORD) == ESP_OK) {
        ESP_LOGI(TAG, "IP: %s", networking_get_ip());
    }

    status_setup();

    // wait just in case wifi and lwip still initializing?
    vTaskDelay(500 / portTICK_PERIOD_MS);

    //TODO Axis Init
    saxis_setup_all();

    //TODO Server Init
    config_server_start();
    command_server_start();

    //TODO move these somewhere else
    configurator_register_tool_handler("saxis", saxis_cfg_init_from_json, saxis_cfg_config_from_json);

    bool s_led_state = 0;
    uint64_t loop_i = 0;
    while(1){
        gpio_set_level(STATUS_GPIO, s_led_state ^= 1);

        if (loop_i % 5 == 0){
            ESP_LOGI(TAG, "Free heap = %u", xPortGetFreeHeapSize());
        }

        loop_i++;

        vTaskDelay(MAIN_PERIOD / portTICK_PERIOD_MS);
    }
}

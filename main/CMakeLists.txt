idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES stepper_driver configurators tcp_servers wifi_setup freertos lwip esp_netif esp_event esp_wifi nvs_flash esp_driver_gpio esp_driver_ledc esp_driver_gptimer
)
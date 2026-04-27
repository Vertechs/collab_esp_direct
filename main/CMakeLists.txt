idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES stepper_driver colab_configure colab_command tcp_servers wifi_setup freertos lwip esp_netif esp_event esp_wifi nvs_flash esp_driver_gpio esp_driver_ledc esp_driver_gptimer
)
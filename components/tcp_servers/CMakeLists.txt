idf_component_register(
    SRCS "helper_f.c" "config_server.c" "command_server.c"
    INCLUDE_DIRS "include" "."
    REQUIRES configurators json freertos lwip esp_netif esp_event esp_wifi esp_netif nvs_flash esp_event
)
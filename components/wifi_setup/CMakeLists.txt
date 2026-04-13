idf_component_register(
    SRCS "wifi_setup.c"
    INCLUDE_DIRS "include"
    REQUIRES freertos lwip esp_netif esp_event esp_wifi esp_netif nvs_flash esp_event
)
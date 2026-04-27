idf_component_register(
    SRCS "cmd_dispatcher.c"
    INCLUDE_DIRS "include"
    REQUIRES tcp_servers freertos esp_event json
)
idf_component_register(
    SRCS "cfg_dispatcher.c"
    INCLUDE_DIRS "include"
    REQUIRES freertos esp_event json
)
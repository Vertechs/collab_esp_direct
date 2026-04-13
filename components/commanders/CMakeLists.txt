idf_component_register(
    SRCS "cmd_interpreter.c"
    INCLUDE_DIRS "include"
    REQUIRES freertos esp_event json
)
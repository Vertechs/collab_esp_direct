idf_component_register(
    SRCS "cfg_dispatcher.c"
    INCLUDE_DIRS "include"
    REQUIRES freertos colab_command esp_event json
)
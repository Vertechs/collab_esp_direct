idf_component_register(
    SRCS "stepper_config.c" "stepper_driver.c"
    INCLUDE_DIRS "include"
    REQUIRES configurators freertos esp_event json esp_event esp_timer esp_driver_gpio esp_driver_ledc esp_driver_gptimer
)
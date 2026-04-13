#pragma once 

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "cJSON.h"

#include "stepper_driver.h"
#include "cfg_dispatcher.h"


err_json_tuple_t saxis_cfg_get_init_template();

err_json_tuple_t saxis_cfg_init_from_json(const char *name, const cJSON *config);
err_json_tuple_t saxis_cfg_config_from_json(const char *name, const cJSON *config);
err_json_tuple_t saxis_cfg_config_set_single(saxis_t *target_axis, cJSON *parameter);
err_json_tuple_t saxis_cfg_get_signal_names();
err_json_tuple_t saxis_cfg_get_signal_from_json(const char *name, const cJSON *config);
esp_err_t saxis_cfg_get_signal_single();
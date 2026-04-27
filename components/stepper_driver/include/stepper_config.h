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

#define JSON_TOOL_NAME "name"
#define JSON_PARAMETER_ARRAY "para"

#define JSON_SAXIS_DIR_PIN "dir_pin"
#define JSON_SAXIS_STEP_PIN "step_pin"
#define JSON_SAXIS_ENABLE_PIN "en_pin"
#define JSON_SAXIS_STEP_PER_EU "steps_per_unit"
#define JSON_SAXIS_UNITS "unit_str"
#define JSON_SAXIS_MICROSTEP "microstep"

#define JSON_SAXIS_SIGNAL_POS "pos_act"
#define JSON_SAXIS_SIGNAL_VEL "vel_act"
#define JSON_SAXIS_SIGNAL_POS_T "pos_tgt"
#define JSON_SAXIS_SIGNAL_VEL_T "vel_tgt"
#define JSON_SAXIS_SIGNAL_MODE "mode_act"
#define JSON_SAXIS_SIGNAL_MODE_CMD "mode_cmd"
#define JSON_SAXIS_SIGNAL_STATE "state"
#define JSON_SAXIS_SIGNAL_ENABLE_CMD "en_cmd"
#define JSON_SAXIS_SIGNAL_RESET_CMD "reset"

#define JSON_SAXIS_FAULT_INFO "fault_info"
#define JSON_SAXIS_FAULT_CODE "fault_code"
#define JSON_SAXIS_FAULT_TIME "fault_time"
#define JSON_SAXIS_FAULT_STAT "fault_stat"
#define JSON_SAXIS_FAULT_RESET "reset"
// TODO, not very clean, 
// not clear to requesting computer what signals do, 
// tree structure is not represented

// static dispatch table better?
typedef err_json_tuple_t (*saxis_cfg_fn)(saxis_t *axis, cJSON *parameter);
typedef struct{
    const char *key;
    saxis_cfg_fn cfg;
}saxis_param_handler_t;

// wrapper functions for driver functions to handle cJSON parsing
static err_json_tuple_t saxis_cfg_dir_pin(saxis_t *axis, cJSON *parameter);
static err_json_tuple_t saxis_cfg_step_pin(saxis_t *axis, cJSON *parameter);
static err_json_tuple_t saxis_cfg_enable_pin(saxis_t *axis, cJSON *parameter);
static err_json_tuple_t saxis_cfg_steps_per_eu(saxis_t *axis, cJSON *parameter);
static err_json_tuple_t saxis_cfg_microstep(saxis_t *axis, cJSON *parameter);
static err_json_tuple_t saxis_cfg_units(saxis_t *axis, cJSON *parameter);

static err_json_tuple_t saxis_cfg_vel_cmd(saxis_t *axis, cJSON *parameter);
static err_json_tuple_t saxis_cfg_pos_cmd(saxis_t *axis, cJSON *parameter);
static err_json_tuple_t saxis_cfg_mode_cmd(saxis_t *axis, cJSON *parameter);
static err_json_tuple_t saxis_cfg_enable(saxis_t *axis, cJSON *parameter);

static err_json_tuple_t saxis_cfg_reset(saxis_t *axis, cJSON *parameter);
static err_json_tuple_t saxis_cfg_fault_info(saxis_t *axis, cJSON *parameter);

static const saxis_param_handler_t param_handlers[] = {
    { JSON_SAXIS_DIR_PIN, saxis_cfg_dir_pin },
    { JSON_SAXIS_STEP_PIN, saxis_cfg_step_pin },
    { JSON_SAXIS_ENABLE_PIN, saxis_cfg_enable_pin },
    { JSON_SAXIS_STEP_PER_EU, saxis_cfg_steps_per_eu },
    { JSON_SAXIS_UNITS, saxis_cfg_units },
    { JSON_SAXIS_MICROSTEP, saxis_cfg_microstep },

    { JSON_SAXIS_SIGNAL_VEL_T, saxis_cfg_vel_cmd },
    { JSON_SAXIS_SIGNAL_POS_T, saxis_cfg_pos_cmd },
    { JSON_SAXIS_SIGNAL_MODE_CMD, saxis_cfg_mode_cmd },
    { JSON_SAXIS_SIGNAL_ENABLE_CMD, saxis_cfg_enable },

    { JSON_SAXIS_FAULT_RESET, saxis_cfg_reset },
    { JSON_SAXIS_FAULT_INFO, saxis_cfg_fault_info }
};

static const int param_handler_count = sizeof(param_handlers) / sizeof(param_handlers[0]);

err_json_tuple_t saxis_cfg_get_init_template();

err_json_tuple_t saxis_cfg_init_from_json(const char *name, const cJSON *config);
err_json_tuple_t saxis_cfg_config_from_json(const char *name, const cJSON *config);
err_json_tuple_t saxis_cfg_config_set_single(saxis_t *target_axis, cJSON *parameter);
err_json_tuple_t saxis_cfg_get_signal_names();
err_json_tuple_t saxis_cfg_get_signal_from_json(const char *name, const cJSON *config);
esp_err_t saxis_cfg_get_signal_single();
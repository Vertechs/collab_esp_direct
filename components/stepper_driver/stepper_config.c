#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "cJSON.h"

#include "stepper_driver.h"
#include "cfg_dispatcher.h"
#include "stepper_config.h"

#define debug 1

extern saxis_t *g_axis_ptr_array[NUM_AXES];

static const char *TAG = "stepper_config";

// init all to zero


//====
//wrapper functions for driver
//Not wrappers, handle getting and setting with fast function from driver
//All should return current value if parameter value is null 
//====

// DIRECTION PIN
static err_json_tuple_t saxis_cfg_dir_pin(saxis_t *axis, cJSON *parameter){
    esp_err_t err = 0;

    if (!cJSON_IsNull(parameter)){
        err = saxis_set_dir_pin(axis, parameter->valueint);
    }

    cJSON *ret_val = cJSON_CreateObject(); //destroyed by root delete call
    cJSON_AddNumberToObject(ret_val, JSON_SAXIS_DIR_PIN, axis->dir_pin);

    return (err_json_tuple_t){err,ret_val};
}

// STEP PIN
static err_json_tuple_t saxis_cfg_step_pin(saxis_t *axis, cJSON *parameter){
    esp_err_t err = 0;

    if (!cJSON_IsNull(parameter)){
        err = saxis_set_step_pin(axis, parameter->valueint);
    }

    cJSON *ret_val = cJSON_CreateObject(); //destroyed by root delete call
    cJSON_AddNumberToObject(ret_val, JSON_SAXIS_STEP_PIN, axis->step_pin);
    
    return (err_json_tuple_t){err,ret_val};
}

// ENABLE PIN
static err_json_tuple_t saxis_cfg_enable_pin(saxis_t *axis, cJSON *parameter){
    esp_err_t err = 0;

    if (!cJSON_IsNull(parameter)){
        err = saxis_set_enable_pin(axis, parameter->valueint);
    }

    cJSON *ret_val = cJSON_CreateObject(); //destroyed by root delete call
    cJSON_AddNumberToObject(ret_val, JSON_SAXIS_ENABLE_PIN, axis->en_pin);
    
    return (err_json_tuple_t){err,ret_val};
}

// STEPS PER UNIT
static err_json_tuple_t saxis_cfg_steps_per_eu(saxis_t *axis, cJSON *parameter){
    esp_err_t err = 0;

    if (!cJSON_IsNull(parameter)){
        err = saxis_set_steps_per_eu(axis, (float)parameter->valuedouble);
    }

    cJSON *ret_val = cJSON_CreateObject(); //destroyed by root delete call
    cJSON_AddNumberToObject(ret_val, JSON_SAXIS_STEP_PER_EU, axis->factor_steps_per_unit);
    
    return (err_json_tuple_t){err,ret_val};
}

// MICROSTEPS
static err_json_tuple_t saxis_cfg_microstep(saxis_t *axis, cJSON *parameter){
    esp_err_t err = 0;

    if (!cJSON_IsNull(parameter)){
        err = saxis_set_microstep(axis, parameter->valueint);
    }

    cJSON *ret_val = cJSON_CreateObject(); //destroyed by root delete call
    cJSON_AddNumberToObject(ret_val, JSON_SAXIS_MICROSTEP, axis->factor_microstep);
    
    return (err_json_tuple_t){err,ret_val};
}

// UNIT STRING
static err_json_tuple_t saxis_cfg_units(saxis_t *axis, cJSON *parameter){
    esp_err_t err = 0;

    if (!cJSON_IsNull(parameter)){
        if (strlen(parameter->valuestring)>MAX_UNITS_LENGTH){
            err = ESP_ERR_INVALID_SIZE;
        }
        strncpy(axis->unit_str, parameter->valuestring,MAX_UNITS_LENGTH);
        axis->unit_str[MAX_UNITS_LENGTH] = '\0';

    }

    cJSON *ret_val = cJSON_CreateObject(); //destroyed by root delete call
    cJSON_AddStringToObject(ret_val, JSON_SAXIS_UNITS, axis->unit_str);
    
    return (err_json_tuple_t){err,ret_val};
}


// VELOCITY COMMAND
static err_json_tuple_t saxis_cfg_vel_cmd(saxis_t *axis, cJSON *parameter){
    esp_err_t err = 0;

    if (!cJSON_IsNull(parameter)){
        err = saxis_set_vel_cmd(axis, parameter->valuedouble);
    }

    cJSON *ret_val = cJSON_CreateObject(); //destroyed by root delete call
    cJSON_AddNumberToObject(ret_val, JSON_SAXIS_SIGNAL_VEL_T, axis->target_velocity_cmd);
    
    return (err_json_tuple_t){err,ret_val};
}

// POSITION COMMAND
static err_json_tuple_t saxis_cfg_pos_cmd(saxis_t *axis, cJSON *parameter){
    esp_err_t err = 0;

    if (!cJSON_IsNull(parameter)){
        err = saxis_set_pos_cmd(axis, parameter->valuedouble);
    }

    cJSON *ret_val = cJSON_CreateObject(); //destroyed by root delete call
    cJSON_AddNumberToObject(ret_val, JSON_SAXIS_SIGNAL_POS_T, axis->target_position_cmd);
    
    return (err_json_tuple_t){err,ret_val};
}

// MODE SWITCH COMMAND
static err_json_tuple_t saxis_cfg_mode_cmd(saxis_t *axis, cJSON *parameter){
    esp_err_t err = 0;

    if (!cJSON_IsNull(parameter)){
        err = saxis_set_mode_cmd(axis, parameter->valueint);
    }

    cJSON *ret_val = cJSON_CreateObject(); //destroyed by root delete call
    cJSON_AddNumberToObject(ret_val, JSON_SAXIS_SIGNAL_MODE_CMD, axis->mode_cmd);
    
    return (err_json_tuple_t){err,ret_val};
}

// ENABLE COMMAND
static err_json_tuple_t saxis_cfg_enable(saxis_t *axis, cJSON *parameter){
    esp_err_t err = 0;

    if (!cJSON_IsNull(parameter)){
       err = saxis_set_enable(axis, parameter->valueint);
    }

    cJSON *ret_val = cJSON_CreateObject(); //destroyed by root delete call
    cJSON_AddNumberToObject(ret_val, JSON_SAXIS_SIGNAL_ENABLE_CMD, axis->enable);
    
    return (err_json_tuple_t){err,ret_val};
}

// RESET COMMAND
static err_json_tuple_t saxis_cfg_reset(saxis_t *axis, cJSON *parameter){

    //no arguments used, assume always asking to reset
    // if (!cJSON_IsNull(parameter)){
    //    err = saxis_reset(axis);
    // }

    esp_err_t err = saxis_reset(axis);

    cJSON *ret_val = cJSON_CreateObject(); //destroyed by root delete call
    cJSON_AddNumberToObject(ret_val, JSON_SAXIS_FAULT_CODE, axis->fault_info.code);
    cJSON_AddNumberToObject(ret_val, JSON_SAXIS_FAULT_TIME, axis->fault_info.time);
    cJSON_AddNumberToObject(ret_val, JSON_SAXIS_FAULT_STAT, !axis->fault_okay);
    
    return (err_json_tuple_t){err,ret_val};
}

// GET FAULTS
static err_json_tuple_t saxis_cfg_fault_info(saxis_t *axis, cJSON *parameter){
    esp_err_t err = ESP_OK;

    // no arguments used
    // if (!cJSON_IsNull(parameter)){
    //    err = saxis_reset(axis);
    // }

    cJSON *ret_val = cJSON_CreateObject(); //destroyed by root delete call
    cJSON_AddNumberToObject(ret_val, JSON_SAXIS_FAULT_CODE, axis->fault_info.code);
    cJSON_AddNumberToObject(ret_val, JSON_SAXIS_FAULT_TIME, axis->fault_info.time);
    cJSON_AddNumberToObject(ret_val, JSON_SAXIS_FAULT_STAT, axis->fault_okay);
    
    return (err_json_tuple_t){err,ret_val};
}

//====
// Main functions
//====


err_json_tuple_t saxis_cfg_get_init_template(){
    return (err_json_tuple_t){ESP_OK, NULL};
    // TODO Build and return JSON structure with parameter names for initialization and config
}

err_json_tuple_t saxis_cfg_config_set_single(saxis_t *target_axis, cJSON *parameter){
    // TODO again not clean, might be a way to centralize this at least in the compiler

    #if debug
    if (cJSON_IsNull(parameter)){
        ESP_LOGI(TAG, "Getting %s", parameter->string);
    }else{
        ESP_LOGI(TAG, "Setting %s", parameter->string);
    }

    #endif

    // NEW static dispatch table
    for (int i = 0; i < param_handler_count; i++) {
        if (strcmp(parameter->string, param_handlers[i].key) == 0) {

            // if value of parameter key is null, return the current value and do not set
            // TODO moving this inside wrapper function since they are not really wrappers anyway
            if (param_handlers[i].cfg != NULL){
                return param_handlers[i].cfg(target_axis,parameter);
            }else{
                break;
            }
        }
    }

    ESP_LOGW(TAG, "No handler for parameter: %s", parameter->string);

    return((err_json_tuple_t){ESP_ERR_INVALID_ARG, NULL});
}


err_json_tuple_t saxis_cfg_init_from_json(const char *name, const cJSON *config){
    // check if name exists in hardware array, return if exists, otherwise allocate a new one     

    #if debug
    ESP_LOGI(TAG, "Init with: %s", cJSON_PrintUnformatted(config));
    #endif

    for(int i=0; i<=NUM_AXES; i++){
        // TODO one of these is NULL
        if(name == NULL){
            #if debug
            ESP_LOGI(TAG, "Name is null");
            #endif
            return((err_json_tuple_t){ESP_ERR_INVALID_ARG, NULL});
        }

        if (g_axis_ptr_array[i]==NULL){continue;}

        if(strcmp(name, g_axis_ptr_array[i]->name_str)==0){
            cJSON *tool = cJSON_CreateObject();
            cJSON_AddStringToObject(tool, JSON_TOOL_NAME, name);
            #if debug
            ESP_LOGI(TAG, "Axis %s existing",name);
            #endif
            return (err_json_tuple_t){ESP_OK, tool};
        }
    }

    // check all items for unitialized axis
    int next_free_index = -1;
    for(int i=0; i<=NUM_AXES; i++){
        if (!g_axis_ptr_array[i]->initialized){
            next_free_index = i;
            break;
        }
    }

    if (next_free_index == -1) {
        #if debug
        ESP_LOGI(TAG, "No free slots in axis array");
        #endif
        return (err_json_tuple_t){ESP_ERR_NOT_ALLOWED, NULL};
    }


    // -----------------
    // Checks passed, start setting up device

    #if debug
    ESP_LOGI(TAG, "Initializing %s at index %d",name,next_free_index);
    #endif


    saxis_t *target_axis = g_axis_ptr_array[next_free_index];

    if (strlen(name)>MAX_NAME_LENGTH){
        #if debug
        ESP_LOGI(TAG, "Name too long %s %d > %d",name,strlen(name),MAX_NAME_LENGTH);
        #endif
        return (err_json_tuple_t){ESP_ERR_INVALID_ARG, NULL};
    }else{
        strncpy(target_axis->name_str, name, MAX_NAME_LENGTH);
        target_axis->name_str[MAX_NAME_LENGTH] = '\0';
    }

    // traverse JSON and set parameters as appropriate
    // should probably mirror struct structure as close as possible

    // getting parameters from calling function, maybe should change?
    //cJSON *parameters = cJSON_GetObjectItem(config, JSON_PARAMETER_ARRAY);
    cJSON *parameter = NULL;
    err_json_tuple_t result = {ESP_OK, NULL};

    // array of results, should be deleted by caller function
    cJSON *return_root = cJSON_CreateArray();

    int fail_count = 0; 

    cJSON_ArrayForEach(parameter, config){
        result = saxis_cfg_config_set_single(target_axis, parameter);
        //TODO actually return results
        if (result.err != ESP_OK){
            fail_count++;
            ESP_LOGW(TAG, "Error setting %s", parameter->string);
        }
        cJSON_AddItemToArray(return_root, result.data);

    }

    // init after config in case any errors should abort the initialization
    esp_err_t init_err = saxis_cfg_initialize(target_axis);

    if (init_err != ESP_OK){
        saxis_cfg_deinitialize(target_axis);
        return (err_json_tuple_t){init_err, NULL};
    }

    return (err_json_tuple_t){(fail_count>1) ? ESP_ERR_INVALID_ARG : ESP_OK, return_root};

    // return JSON with success or failure in fields
}

//
// TODO ERROR IN THUS FUNCTION
//
//
err_json_tuple_t saxis_cfg_config_from_json(const char *name, const cJSON *config){
     // check if name exists in hardware array, return if exists, otherwise allocate a new one     

    #if debug
    ESP_LOGI(TAG, "Config with: %s", cJSON_PrintUnformatted(config));
    #endif

    if(name == NULL){
        #if debug
        ESP_LOGI(TAG, "Name is null");
        #endif
        return((err_json_tuple_t){ESP_ERR_INVALID_ARG, NULL});
    }

    int axis_index = -1; 
    for(int i=0; i<=NUM_AXES; i++){

        if (g_axis_ptr_array[i]==NULL){continue;}

        if(strcmp(name, g_axis_ptr_array[i]->name_str)==0){
            #if debug
            ESP_LOGI(TAG, "Found %s at %d",name,i);
            #endif
            axis_index = i;
            break;
        }
    }

    if (axis_index == -1){
        ESP_LOGW(TAG, "Axis %s not found",name);
        return (err_json_tuple_t){ESP_ERR_NOT_FOUND, NULL};
    }

    // -----------------
    // Checks passed, start setting up device

    saxis_t *target_axis = g_axis_ptr_array[axis_index];

    // traverse JSON and set parameters as appropriate
    // should probably mirror struct structure as close as possible

    // getting parameters from calling function, maybe should change?
    //cJSON *parameters = cJSON_GetObjectItem(config, JSON_PARAMETER_ARRAY);
    cJSON *parameter = NULL;
    err_json_tuple_t result = {ESP_OK, NULL};

    // array of results, should be deleted by caller function
    cJSON *return_root = cJSON_CreateArray();

    int fail_count = 0; 

    cJSON_ArrayForEach(parameter, config){
        result = saxis_cfg_config_set_single(target_axis, parameter);
        //TODO actually return results
        if (result.err != ESP_OK){
            fail_count++;
            ESP_LOGW(TAG, "Error setting %s", parameter->string);
        }
        cJSON_AddItemToArray(return_root, result.data);

    }

    return (err_json_tuple_t){(fail_count>1) ? ESP_ERR_INVALID_ARG : ESP_OK, return_root};
}



err_json_tuple_t saxis_cfg_get_signal_names(){
    return (err_json_tuple_t){ESP_FAIL, NULL};
    // TODO build and return JSON with all available signal names and access levels
}


err_json_tuple_t saxis_cfg_get_signal_from_json(const char *name, const cJSON *config){
    // traverse JSON and find pointer and size of each requested signal
    // call to single get function to get pointer and size?
    // call to cfg_signal_get to add to registry

    // return JSON with signal register index, type, and size for each requested
    return (err_json_tuple_t){ESP_FAIL, NULL};
}

esp_err_t saxis_cfg_get_signal_single(){
    return 0;
}
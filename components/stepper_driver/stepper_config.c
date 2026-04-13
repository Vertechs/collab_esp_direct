#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "cJSON.h"

#include "stepper_driver.h"
#include "cfg_dispatcher.h"

#define debug 1

#define JSON_TOOL_NAME "name"
#define JSON_PARAMETER_ARRAY "para"

#define JSON_SAXIS_DIR_PIN "dir_pin"
#define JSON_SAXIS_STEP_PIN "step_pin"
#define JSON_SAXIS_ENABLE_PIN "en_pin"
#define JSON_SAXIS_STEP_PER_EU "steps_per_unit"
#define JSON_SAXIS_UNITS "units"

#define JSON_SAXIS_SIGNAL_POS "pos_act"
#define JSON_SAXIS_SIGNAL_VEL "vel_act"
#define JSON_SAXIS_SIGNAL_POS_T "pos_tgt"
#define JSON_SAXIS_SIGNAL_VEL_T "vel_tgt"
#define JSON_SAXIS_SIGNAL_MODE "mode_act"
#define JSON_SAXIS_SIGNAL_MODE_CMD "mode_cmd"
#define JSON_SAXIS_SIGNAL_STATE "state"
#define JSON_SAXIS_SIGNAL_ENABLE_CMD "en_cmd"
// TODO, not very clean, 
// not clear to requesting computer what signals do, 
// tree structure is not represented

extern saxis_t *g_axis_ptr_array[NUM_AXES];

static const char *TAG = "SAxisConfig";

// init all to zero

err_json_tuple_t saxis_cfg_get_init_template(){
    return (err_json_tuple_t){ESP_OK, NULL};
    // TODO Build and return JSON structure with parameter names for initialization and config
}

err_json_tuple_t saxis_cfg_config_set_single(saxis_t *target_axis, cJSON *parameter){
    // TODO again not clean, might be a way to centralize this at least in the compiler

    #if debug
    ESP_LOGI(TAG, "Setting %s", parameter->valuestring);
    #endif

    // TODO not accessing the value here correctly
    // call set functions for process critical values
    if (strcmp(parameter->valuestring, JSON_SAXIS_DIR_PIN)==0){
        return(saxis_set_dir_pin(target_axis,(gpio_num_t)parameter->valueint));

    }else if (strcmp(parameter->valuestring, JSON_SAXIS_STEP_PIN)==0){
        return(saxis_set_step_pin(target_axis,parameter->valueint));

    }else if (strcmp(parameter->valuestring, JSON_SAXIS_ENABLE_PIN)==0){
        return(saxis_set_enable_pin(target_axis,parameter->valueint));

    }else if (strcmp(parameter->valuestring, JSON_SAXIS_STEP_PER_EU)==0){
        return(saxis_set_steps_per_eu(target_axis,(float)parameter->valuedouble));

    // not process critical, set directly
    }else if (strcmp(parameter->valuestring, JSON_SAXIS_UNITS)==0){

        if (strlen(parameter->valuestring)>MAX_UNITS_LENGTH){
            strcpy(target_axis->unit_str,parameter->valuestring);
            return((err_json_tuple_t){ESP_OK, NULL});

        }else{
            return((err_json_tuple_t){ESP_ERR_INVALID_ARG, NULL});
        }

    }

    return((err_json_tuple_t){ESP_ERR_INVALID_ARG, NULL});
}


err_json_tuple_t saxis_cfg_init_from_json(const char *name, const cJSON *config){
    // check if name exists in hardware array, return if exists, otherwise allocate a new one     
    
    for(int i=0; i<=NUM_AXES; i++){
        // TODO one of these is NULL
        if(name == NULL){return((err_json_tuple_t){ESP_ERR_INVALID_ARG, NULL});}

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
        if (g_axis_ptr_array[i]==NULL){continue;}

        if (!g_axis_ptr_array[i]->initialized){
            next_free_index = i;
            break;
        }
    }

    if (next_free_index == -1) {return (err_json_tuple_t){ESP_ERR_INVALID_ARG, NULL};}

    #if debug
    ESP_LOGI(TAG, "Initializing %s at index %d",name,next_free_index);
    #endif

    saxis_t *target_axis = g_axis_ptr_array[next_free_index];

    if (strlen(name)>MAX_NAME_LENGTH){
        strcpy(target_axis->name_str,name);
    }else{
        return (err_json_tuple_t){ESP_ERR_INVALID_ARG, NULL};
    }

    // traverse JSON and set parameters as appropriate
    // should probably mirror struct structure as close as possible

    cJSON *parameters = cJSON_GetObjectItem(config, JSON_PARAMETER_ARRAY);
    cJSON *parameter = NULL;
    err_json_tuple_t result = {ESP_OK, NULL};

    int fail_count = 0; 

    cJSON_ArrayForEach(parameter, parameters){
        result = saxis_cfg_config_set_single(target_axis, parameter);
        //TODO actually return results
        if (result.err != ESP_OK){
            fail_count++;
        }
    }

    // init after config in case any errors should abort the initialization
    esp_err_t init_err = saxis_cfg_initialize(target_axis);

    return (err_json_tuple_t){fail_count+(init_err!=ESP_OK), NULL};

    // return JSON with success or failure in fields
}

err_json_tuple_t saxis_cfg_config_from_json(const char *name, const cJSON *config){
    // traverse JSON and set parameters as appropriate
    // should probably mirror struct structure as close as possible

    int target_index = -1;
    for(int i=0; i<=NUM_AXES; i++){
        if(strcmp(name, g_axis_ptr_array[i]->name_str)==0){
            target_index = i;
            break;
        }
    }

    if (target_index == -1) {return (err_json_tuple_t){ESP_ERR_INVALID_ARG, NULL};}

    saxis_t *target_axis = g_axis_ptr_array[target_index];

    // traverse JSON and set parameters as appropriate
    // should probably mirror struct structure as close as possible

    cJSON *parameters = cJSON_GetObjectItem(config, JSON_PARAMETER_ARRAY);
    cJSON *parameter = NULL;
    err_json_tuple_t result = {ESP_OK, NULL};

    int fail_count = 0; 

    cJSON_ArrayForEach(parameter, parameters){
        result = saxis_cfg_config_set_single(target_axis, parameter);
        //TODO actually return results
        if (result.err != ESP_OK){
            fail_count++;
        }
    }
    return (err_json_tuple_t){fail_count, NULL};

    // return JSON with success or failure in fields
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
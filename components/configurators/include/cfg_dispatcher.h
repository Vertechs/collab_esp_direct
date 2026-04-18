#pragma once

#include <string.h>

#include "cJSON.h"
#include "esp_err.h"

// returning ESP error for immediate logging, and JSON data for return back to client
typedef struct{
    esp_err_t err;
    cJSON *data;  //ownership of calling function, delete after appending to return JSON
}err_json_tuple_t;

typedef struct{
    esp_err_t err;
    float value;
}err_val_tuple_t;

// initialization function, should implement similar to config functions but allowed
// to allocate new entries in hardware arrays
typedef err_json_tuple_t (*tool_init_fn)(const char *name, const cJSON *config);

// configuration function, walk through provided parameters and assign as appropriate
// should return an error if "name" does not already exist in hardware arrays
typedef err_json_tuple_t (*tool_config_fn)(const char *name, const cJSON *config);
// TODO return JSON data from each of these for return to client


typedef struct {

} available_signal_t;//NOT TODO??
// do need a handler function, can be single per tool though, parse the json data for whatever signals
// are being asked for, have a single function per tool struct to get any avilable signals
//instead:
typedef err_json_tuple_t (*tool_signal_fn)(const char *name, const cJSON *signal_config);


typedef struct {
    char type[32];
    tool_init_fn init;
    tool_config_fn config;
} cfg_dispatch_table_item_t;




// root and return_root cJSON owned and deleted by calling function
err_json_tuple_t configurator_dispatch(uint8_t msg_type, cJSON *root);
esp_err_t configurator_register_tool_handler(const char *type, tool_init_fn init, tool_config_fn config);
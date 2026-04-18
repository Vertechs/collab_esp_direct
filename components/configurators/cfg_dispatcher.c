#include "cfg_dispatcher.h"

#include <string.h>
#include <stdbool.h>

#include "cJSON.h"
#include "esp_err.h"
#include "esp_log.h"

#define MAX_TOOL_HANDLERS 64

#define debug 1

static const char *TAG = "cfg_dispatch";

static cfg_dispatch_table_item_t cfg_dispatch_table[MAX_TOOL_HANDLERS];
static int handler_count = 0;

err_json_tuple_t configurator_dispatch(uint8_t msg_type, cJSON *root){

    // get root tool array
    cJSON *tool_array = cJSON_GetObjectItem(root, "tools");
    if (!cJSON_IsArray(tool_array)){
        ESP_LOGE(TAG, "Expected array of tool configs at JSON root");
        return (err_json_tuple_t){ESP_ERR_INVALID_ARG,NULL};
    }

    // will be deleted by caller function
    cJSON *return_root = cJSON_CreateArray();

    // loop through all tool config objects in array from root
    const cJSON *tool_cfg = NULL;
    int loop_ind = 0; 
    int failed_count = 0;
    cJSON_ArrayForEach(tool_cfg, tool_array){
        // get device info from tool list, only what is needed to send to the correct handler
        // all deleted along with root by caller function
        cJSON *type_item = cJSON_GetObjectItem(tool_cfg, "type");
        cJSON *name_item = cJSON_GetObjectItem(tool_cfg, "name");
        cJSON *is_init = cJSON_GetObjectItem(tool_cfg, "init"); // if null, assume this is not an init call
        cJSON *cfg_data = cJSON_GetObjectItem(tool_cfg, "para");

        #if debug
        ESP_LOGI(TAG, "Configuring \'%s\' as type \'%s\'", name_item->valuestring, type_item->valuestring);
        #endif

        if (!cJSON_IsString(type_item) || !cJSON_IsString(name_item)) {
            ESP_LOGW(TAG, "Skipping device configuration (%d), missing required fields", loop_ind);
            failed_count++;
            continue;
        }

        const char *type_str = type_item->valuestring;
        const char *name_str = name_item->valuestring;

        // check dispatch table for the correct handler
        err_json_tuple_t handler_return = {ESP_OK, NULL};
        bool handler_found = 0; //nested loops hurt brain
        for (int i=0; i < handler_count; i++){
            if (strcmp(cfg_dispatch_table[i].type, type_str) == 0){
                #if debug
                ESP_LOGI(TAG, "Handler found for type \'%s\', init=%d", cfg_dispatch_table[i].type, cJSON_IsTrue(is_init));
                #endif
                handler_found = true;

                // IsTrue returns false if is_init is null or not true, so assume this is a config call if
                // init is false or not provided
                if (cJSON_IsTrue(is_init)){
                    handler_return = cfg_dispatch_table[i].init(name_str, cfg_data);
                }else{
                    handler_return = cfg_dispatch_table[i].config(name_str, cfg_data);
                }

            } // else continue..
        }

        // log if any errors occurred, increase failed counter for final return
        if (!handler_found){
            ESP_LOGW(TAG, "No handler defined for tool type: %s", type_str);
            failed_count++;
        }
        else if (handler_return.err != ESP_OK){
            ESP_LOGW(TAG, "Error in handler function for: %s, type:%s, (err=%x)", name_str, type_str, handler_return.err);
            failed_count++;
        }


        // TODO add returned JSON data from handlers to final return data
        // cJSON_add_to_array_function
        cJSON_AddItemToArray(return_root, handler_return.data); // append even if still NULL


        loop_ind++;
    }

    // root cJSON deleted by calling function in tcp config_server
    return (err_json_tuple_t){ESP_OK,return_root};
}



// add device init and config handler functions to the global dispatch table
// TODO, dynamic assignment necessary?
esp_err_t configurator_register_tool_handler(const char *type, tool_init_fn init, tool_config_fn config){
    if (handler_count >= MAX_TOOL_HANDLERS){
        ESP_LOGE(TAG, "Tool dispatch table full");
        return ESP_ERR_NOT_ALLOWED;
    }

    ESP_LOGI(TAG, "Handler register for tool type \"%s\"", type);
    strlcpy(cfg_dispatch_table[handler_count].type,
            type, 
            sizeof(cfg_dispatch_table[handler_count].type)
        );
    cfg_dispatch_table[handler_count].init = init;
    cfg_dispatch_table[handler_count].config = config;
    handler_count ++;
    return ESP_OK;
}




// TODO function for de-registering handler functions?
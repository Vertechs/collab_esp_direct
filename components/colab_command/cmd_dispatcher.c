#include "cmd_dispatcher.h"
#include "tcp_servers.h"

// tcp_server to cmd_signal_cfg if signal add request (<128)
// cmd_signal_cfg JSON, looks up device type for handler in dynamic dispatch table
// cmd_signal_cfg sends JSON or just list of signal names to device command handler
// device command handler returns data length, access, and read and write functions (if applicable)
// those arguments are sent to cmd_dispatcher to add to signal dispatch table
// cmd_dispatcher returns index of that signal to cmd_signal_cfg
// cmd_signal_cfg places that index in a new JSON object with a key for the signal name
// finally, here returns JSON to tcp_server

// tcp_server to cmd_dispatch if actual signal message (>128)
// cmd_dispatch O(1) lookup by index, checks message data length, passes to function in table
// cmd_dispatch optionally returns requested data or command status to tcp_server

#define MAX_SIGNAL_REGISTERS 2048


//TODO standardize naming
static const char *TAG = "cmd_dispatcher";

// Allocate signal lookup table (2048?)
static signal_register_t signal_register_table[MAX_SIGNAL_REGISTERS];

//TODO
void registry_init(void) {
    memset(signal_register_table, 0, sizeof(signal_register_table));  // sets all ptrs to NULL
    ESP_LOGI(TAG, "registry initialized, capacity %d", MAX_SIGNAL_REGISTERS);
}

esp_err_t signal_register_dispatch(uint16_t index, uint8_t *packet_data, uint8_t *reply_buffer, message_types_t type){
    if (type == TCP_CMD_READ_SIGNAL){
        signal_register_table[index].read(index, reply_buffer)
        
    }

}

uint16_t signal_register_add(void *device_ctx, uint16_t data_length, signal_access_t access, cmd_write_handler write_handler, cmd_read_handler read_handler)
{
    uint16_t new_index = MAX_SIGNAL_REGISTERS;

    // Dont check to see if signal already exists, assume JSON parsing function handles this

    // get first available entry in table (ctx pointer is null)
    for (int i = 0; i<MAX_SIGNAL_REGISTERS; i++){
        if (signal_register_table[i].device_ctx == NULL){
            new_index = i;
            break;
        }
    }

    signal_register_t new_entry = {.device_ctx = device_ctx, }

    signal_register_table[new_index] = new_entry


    return new_index
}





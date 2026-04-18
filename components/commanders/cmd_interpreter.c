#include "cmd_interpreter.h"

#define MAX_SIGNAL_REGISTERS 2048

//TODO standardize naming
static const char *TAG = "Ccmd_interpreter";

// Allocate signal lookup table (2048?)
static signal_register_t signal_register_table[MAX_SIGNAL_REGISTERS];

//TODO
void registry_init(void) {
    memset(signal_register_table, 0, sizeof(signal_register_table));  // sets all ptrs to NULL
    ESP_LOGI(TAG, "registry initialized, capacity %d", MAX_SIGNAL_REGISTERS);
}

esp_err_t signal_register_dispatch(){
    // TODO????
    // may need another dynamic dispatch table... maybe should move this section to the configurator
}

esp_err_t signal_register_add(void *ptr, uint16_t len, signal_access_t access)
{
    //TODO
    //Loop through signal register table and compare ptr, if exists, return index
    //Else create new entry in table
    //ptr must be to extern??
}

esp_err_t signal_register_set()
{

}

get_result_t signal_register_get()
{
    //check if incoming request matches register table

}
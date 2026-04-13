#pragma once
 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
 
// Ports for config(json) and command(message) data
#define TCP_COMMAND_PORT     5000
#define TCP_CONFIG_PORT       5001
 
// [msg_type 1 byte][msg_len 4 bytes][payload N bytes]

// FreeRTOS task configs TODO, check against motor control
#define TCP_COMMAND_TASK_PRIORITY    8       // High — keeps command path responsive
#define TCP_CONFIG_TASK_PRIORITY      4      // Low  — JSON parse can be preempted
#define TCP_COMMAND_TASK_STACK_SIZE  4096
#define TCP_CONFIG_TASK_STACK_SIZE   8192    // Larger: cJSON allocates on the heap
                                             // but stack usage is also higher
 
// Receive buffer sizes
#define COMMAND_RECV_BUF_SIZE    256
#define CONFIG_RECV_BUF_SIZE      2048
 
// public functions
void config_server_start(void);
void command_server_start(void);

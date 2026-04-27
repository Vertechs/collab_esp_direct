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
 
typedef enum{
    // command message < 128
    TCP_CMD_WRITE_SIGNAL = 0x01,
    TCP_CMD_READ_SIGNAL = 0x02, // TODO allow subscription
    TCP_CMD_WRITE_SIGNAL_ACK = 0x11, // same as write but reply with an ack packet with error code
    TCP_CMD_WRITE_SIGNAL_REPLY = 0x12, // same as write but reply with the result of the write 

    // config messages > 128
    TCP_CFG_GENERIC = 0x81, // for normal JSON parameter config
    TCP_CFG_GET_SIGNAL = 0x91, // get index of signal (will create new table entry if not existing)
    TCP_CFG_FREE_SIGNAL = 0x92, // allow signal table entry to be freed (TODO how handle this for multiple clients?)


}message_types_t;


// Receive buffer sizes
#define COMMAND_RECV_BUF_SIZE    256
#define CONFIG_RECV_BUF_SIZE      2048
 
// public functions
void config_server_start(void);
void command_server_start(void);

#pragma once

#include <string.h>

#include "cJSON.h"
#include "esp_err.h"

#define DESC_MAX_LENGTH 64

typedef enum{
    ACCESS_RO,
    ACCESS_RW,
    ACCESS_WO
}signal_access_t;

// write and read functions, read return length of data written to out buffer
typedef esp_err_t (*cmd_write_handler)(void *in_data, uint16_t in_data_length, void *device_ctx);
typedef uint16_t (*cmd_read_handler)(void *out_data, uint16_t out_length_max, void *device_ctx);

typedef struct {
    signal_access_t access;
    uint8_t data_length;
    cmd_write_handler write;
    cmd_read_handler read;
    void *device_ctx;
}signal_register_t;


// Not this, pointers or table to set/get functions, no direct variable access
// typedef struct{
//     void *value;
//     uint16_t len;
//     esp_err_t err;
// }get_result_t;

// typedef struct{
//     // no type, assume caller knows?
//     signal_access_t access;
//     uint16_t len;
//     void *ptr;
//     char desc[DESC_MAX_LENGTH];
// }signal_register_t;


// extend the above or merge for stream data?
// probably better to have separate table for streaming data, can be smaller
// but points to signal from signal register table so not duplicating
typedef struct{
    signal_register_t *signal;
    uint16_t packet_interval; // in us
    uint64_t last_sent;
}signal_stream_t;

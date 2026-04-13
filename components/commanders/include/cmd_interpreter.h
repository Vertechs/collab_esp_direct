#pragma once

#include <string.h>

#include "cJSON.h"
#include "esp_err.h"

#define DESC_MAX_LENGTH

typedef enum{
    ACCESS_RO,
    ACCESS_RW,
    ACCESS_WO
}signal_access_t;

typedef struct{
    void *value;
    uint16_t len;
    esp_err_t err;
}get_result_t;

typedef struct{
    // no type, assume caller knows?
    signal_access_t access;
    uint16_t len;
    void *ptr;
    char desc[DESC_MAX_LENGTH];
}signal_register_t;


// extend the above or merge for stream data?
// probably better to have separate table for streaming data, can be smaller
// but points to signal from signal register table so not duplicating
typedef struct{
    signal_register_t *signal;
    uint16_t packet_interval; // in us
    uint64_t last_sent;
}signal_stream_t;

#pragma once

#include "esp_err.h"

// Initialize WiFi and block until connected or failed.
// Returns ESP_OK on successful connection.
esp_err_t networking_wifi_connect(const char *ssid, const char *password);

// Returns the current IP address as a string (e.g. "192.168.1.50").
// Returns NULL if not connected.
const char *networking_get_ip(void);
#include "tcp_servers.h"
#include "helper_f.h"
#include "cfg_dispatcher.h"
 
#include <string.h>
#include <errno.h>

#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "cJSON.h"

#define debug 1

static const char *TAG = "config_server";


//===================
// config tcp message client task
//===================

static void handle_config_client(int client_sock){
    char buf[CONFIG_RECV_BUF_SIZE];

    while(1){
        // get header data from socket, message type (1byte), data length(3byte, )
        int received = recv(client_sock, buf, 5, MSG_WAITALL);

        if (received <= 0){
            ESP_LOGI(TAG, "Client disconnected (recv=%d)", received);
            break;
        }

        uint8_t msg_type = buf[0];

        uint32_t data_length = (buf[1] << 24) | (buf[2] << 16) | (buf[3] << 8) | buf[4]; // big-endian

        if (data_length > CONFIG_RECV_BUF_SIZE-1){
            ESP_LOGE(TAG, "Message length exceeds buffer, (%d bytes), dropping client", data_length);
            break;
        }

        if (data_length > 0){
            received = recv(client_sock, buf, data_length, MSG_WAITALL);
            if (received <= 0){
                ESP_LOGI(TAG, "Client disconnected (recv=%d)", received);
                break;
            }
        }

        // check if valid config message type
        if (msg_type < 128){
            ESP_LOGE(TAG, "Message type not a config message (%d < 128)", msg_type);
            break;
        }
        
        buf[data_length] = '\0';

        cJSON *root = cJSON_Parse(buf);
        if (root == NULL){
            ESP_LOGE(TAG, "JSON config message parse error, dropping client");
            break;
        }

        // TODO send JSON to config handler
        err_json_tuple_t dispatch_reply = configurator_dispatch(msg_type, root);

        // TODO send ack or failed
        char *ack_msg = cJSON_PrintUnformatted(dispatch_reply.data);
        #if debug
            ESP_LOGI(TAG, "Sending %u bytes", strlen(ack_msg));
        #endif
        send(client_sock, ack_msg, strlen(ack_msg), 0);

        // TODO send reply JSON


        // free JSON data from client and from 
        cJSON_Delete(dispatch_reply.data);
        cJSON_free(ack_msg);
        cJSON_Delete(root);

    }
}


static void tcp_config_server_task(void *pvParameters){
    // bind port and start tcp state machine listening 
    int listen_sock = create_listen_socket(TCP_CONFIG_PORT, TAG);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Failed to create listen socket — task exiting");
        vTaskDelete(NULL);
        return;
    }

    while (1){
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
 
        ESP_LOGI(TAG, "Waiting for connection...");
        int client_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock < 0) {
            ESP_LOGE(TAG, "accept() failed: errno %d", errno);
            continue;
        }
 
        ESP_LOGI(TAG, "Client connected: %s", inet_ntoa(client_addr.sin_addr));
        
        handle_config_client(client_sock);
 
        close(client_sock);
        ESP_LOGI(TAG, "Client socket closed: %s", inet_ntoa(client_addr.sin_addr));
    }
}

// wrapper to start server in RTOS task with configured stack size and priority
void config_server_start(void){
    xTaskCreate(tcp_config_server_task, "TCP Config", TCP_CONFIG_TASK_STACK_SIZE, NULL, TCP_CONFIG_TASK_PRIORITY, NULL);
}
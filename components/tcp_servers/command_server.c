#include "tcp_servers.h"
#include "helper_f.h" 

#include <string.h>
#include <errno.h>

#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "cJSON.h"

#define debug 1

static const char *TAG_CMD = "command_server";

//===================
//command tcp message server task
//sleep on wait for recv()
//===================

static void handle_command_client(int client_sock){
    uint8_t buf[COMMAND_RECV_BUF_SIZE];

    while (1){

        // Read header bytes, same for command and config messages, type=1, length=2
        int received = recv(client_sock, buf, 3, MSG_WAITALL);
        if (received <= 0){
            ESP_LOGI(TAG_CMD, "Client disconnected (recv'd=%d)",received);
            break;
        }

        uint8_t msg_type = buf[0];
        uint16_t data_length = (buf[1] << 8) | buf[2]; //big endian, minus first 3 bytes

        if (data_length > COMMAND_RECV_BUF_SIZE){
            ESP_LOGE(TAG_CMD, "Payload exceeds buffer (%d bytes), dropping client",data_length);
            break;
        }

        if (data_length > 0){
            // move message data into buffer, overwrite header
            received = recv(client_sock, buf, data_length, MSG_WAITALL);

            if (received <=0){
                ESP_LOGI(TAG_CMD, "Client disconnected (recv'd=%d)",received);
                break;
            }
        }

        // check if valid command message type, after pulling data from buffer?
        if (msg_type < 128){
            ESP_LOGE(TAG_CMD, "Message type not a command message (%d > 128)", msg_type);
            break;
        }

        // TODO, check if correct message type

        // TODO, send to command interpreter, allow for active connections

        // TODO, send ack or failed to client

    }
}

//=========
//Command tcp message server
//sleep on wait for accept()
//=========
static void tcp_command_server_task(void *pvParameters)
{
    // create and bind new socket
    int listen_sock = create_listen_socket(TCP_COMMAND_PORT, TAG_CMD);
    if (listen_sock < 0) {
        ESP_LOGE(TAG_CMD, "Failed to create listen socket — task exiting");
        vTaskDelete(NULL);
        return;
    }

    // main task loop, wait for accept()
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
 
        ESP_LOGI(TAG_CMD, "Waiting for connection...");
        
        // accept(), wait for client connection, retry loop if accept() does not return
        int client_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock < 0) {
            ESP_LOGE(TAG_CMD, "accept() failed: errno %d", errno);
            continue;
        }
 
        ESP_LOGI(TAG_CMD, "Client connected: %s", inet_ntoa(client_addr.sin_addr));
        
        handle_command_client(client_sock);
 
        close(client_sock);
        ESP_LOGI(TAG_CMD, "Client socket closed: %s", inet_ntoa(client_addr.sin_addr));
    }

}

// wrapper to start server in RTOS task with configured stack size and priority
void command_server_start(void){

    xTaskCreate(tcp_command_server_task, "TCP Config", TCP_COMMAND_TASK_STACK_SIZE, NULL, TCP_COMMAND_TASK_PRIORITY, NULL);
}
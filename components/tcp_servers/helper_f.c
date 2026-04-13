#include "tcp_servers.h"
#include "helper_f.h"
 
#include <string.h>
#include <errno.h>

#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

// create a new socket, bind and listen on a port
// return the listening socket
int create_listen_socket(uint16_t port, const char *tag){
    // IPV4, stream (TCP), TCP, returns handle
    int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_sock < 0){
        ESP_LOGE(tag, "Failed to create socket, err: %d",errno); //errno, thread local int
        return -1;
    }

    // IMPORTANT, else can lock up if socket open when ESP resboots
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,  // IPV4
        .sin_addr.s_addr = htonl(INADDR_ANY), // host to network byte order of 0.0.0.0 // bind to all network interfaces
        .sin_port = htons(port) //  "" : port
    };

    // Bind port on all network interfaces
    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        ESP_LOGE(tag, "Bind failed on port %d, errno:%d", port, errno);
        close(listen_sock);
        return -1;
    }

    // Start listening on port, backlog of 4 connections
    if (listen(listen_sock, 4) < 0){
        ESP_LOGE(tag, "Listen failed on port %d, errno:%d", port, errno);
        close(listen_sock);
        return -1;
    }

    ESP_LOGI(tag, "Listening on port %d",port);
    return listen_sock;

}


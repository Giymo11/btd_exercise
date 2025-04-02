/*
 * ESP32 TCP Client - Improved version with CONFIG_EXAMPLE_IPV4 assumption
 */

#include <stdio.h>       // For snprintf
#include <stdlib.h>      // Standard library functions (not strictly needed here anymore)

#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>       // struct addrinfo, gai_strerror() if using DNS names
#include <arpa/inet.h>   // inet_pton()

#include "sdkconfig.h" // For Kconfig options
#include "esp_netif.h"   // Networking Interface
#include "esp_log.h"     // Logging framework

#include "freertos/FreeRTOS.h" // FreeRTOS API
#include "freertos/task.h"     // Task management
#include "esp_random.h"  // ESP32 Random number generator


// Configuration checks 
#if !defined(CONFIG_EXAMPLE_IPV4)
#error "This application requires CONFIG_EXAMPLE_IPV4 to be defined"
#endif


#define HOST_IP_ADDR CONFIG_EXAMPLE_IPV4_ADDR
#define PORT CONFIG_EXAMPLE_PORT
#define RECONNECT_DELAY_MS 5000

#define MAX_RX_SIZE 128
#define MAX_TX_SIZE 32

static const char *TAG = "TCP_CLIENT";
static char rx_buffer[MAX_RX_SIZE];
static char tx_buffer[MAX_TX_SIZE];

static int connect_to_server(const char *host_ip, int port) {
    int addr_family = AF_INET;
    int ip_protocol = IPPROTO_IP;

    struct sockaddr_in dest_addr;
    int sock = -1;
    int err = 0;
    
    /* Set up IPv4 connection parameters */
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = addr_family;
    dest_addr.sin_port = htons(port);
    
    /* Convert IP address from string to binary form */
    if (inet_pton(addr_family, host_ip, &dest_addr.sin_addr) != 1) {
        ESP_LOGE(TAG, "Invalid IP address format");
        return -1;
    }
    
    sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return -1;
    }
    
    ESP_LOGI(TAG, "Socket created, attempting to connect to %s:%d", host_ip, port);

    /* Connect to server */
    err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
        close(sock);
        return -1;
    }
    
    ESP_LOGI(TAG, "Successfully connected to server");
    return sock;
}

static int send_message(int sock, const char *message) {
    int err = send(sock, message, strlen(message), 0);
    if (err < 0) {
        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
        return -1;
    }
    return err;
}

static int receive_response(int sock, char *buffer, size_t buffer_size) {
    int len = recv(sock, buffer, buffer_size - 1, 0);
    
    if (len < 0) {
        ESP_LOGE(TAG, "recv failed: errno %d", errno);
        return -1;
    } else if (len == 0) {
        ESP_LOGW(TAG, "Connection closed by server");
        return 0;
    } else {
        buffer[len] = '\0'; // Null-terminate the received data
        ESP_LOGI(TAG, "Received %d bytes: %s", len, buffer);
        return len;
    }
}

static int task_one(int sock) {
    vTaskDelay(pdMS_TO_TICKS(1000)); // 1 second delay
    int random_num = esp_random();
    
    /* Use snprintf instead of itoa for safer, more portable conversion */
    snprintf(tx_buffer, MAX_TX_SIZE, "%d", random_num);

    return send_message(sock, tx_buffer);
}


void tcp_client(void) {
    char host_ip[] = HOST_IP_ADDR;
    int sock = -1;
    bool connected = false;
    
    ESP_LOGI(TAG, "TCP client task starting...");
    
    while (1) {
        if (!connected) {
            sock = connect_to_server(host_ip, PORT);
            if (sock >= 0) {
                connected = true;
            } else {
                vTaskDelay(pdMS_TO_TICKS(RECONNECT_DELAY_MS)); // Wait before retrying
                continue;
            }
        }

        int task_res = task_one(sock);
        if (task_res < 0) {
            connected = false;
            close(sock);
            continue;
        }
        
        if (receive_response(sock, rx_buffer, MAX_RX_SIZE) < 0) {
            connected = false;
            close(sock);
            continue;
        }
    }
    
    /* This code is not reached due to the infinite loop,
       but included for completeness */
    if (sock != -1) {
        ESP_LOGI(TAG, "Shutting down socket...");
        shutdown(sock, 0);
        close(sock);
    }
}

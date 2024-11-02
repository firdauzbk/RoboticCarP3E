#include <stdio.h>
#include "buddy1.h"

#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/netif.h"

#define WIFI_SSID "WenJie (2)"
#define WIFI_PASSWORD "qx25fuhutxvx9"
#define TCP_PORT 4242
#define DEBUG_printf printf
#define BUF_SIZE 2048

// Define the telemetry data struct to store received information
typedef struct {
    char direction[40];  // Holds the direction command (e.g., "Forward Right")
    int speed;           // Holds the speed value
} TelemetryData;

TelemetryData telemetry_data = {0};  // Initialize telemetry_data

// Struct to track TCP server state
typedef struct TCP_SERVER_T_ {
    struct tcp_pcb *server_pcb;
    struct tcp_pcb *client_pcb;
    bool complete;
    uint8_t buffer_recv[BUF_SIZE];
    int recv_len;
} TCP_SERVER_T;

TCP_SERVER_T *persistent_state = NULL;  // Persistent state for sending data

// Function prototypes
static err_t tcp_server_close(void *arg);
static TCP_SERVER_T* tcp_server_init(void);
static bool tcp_server_open(TCP_SERVER_T *state);
static err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err);
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static err_t send_data_to_target(const char *direction, int speed);

// Initialize the TCP server state
static TCP_SERVER_T* tcp_server_init(void) {
    TCP_SERVER_T *state = calloc(1, sizeof(TCP_SERVER_T));
    if (!state) {
        DEBUG_printf("Failed to allocate state\n");
        return NULL;
    }
    return state;
}

// Close the TCP connection and reset server state
static err_t tcp_server_close(void *arg) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (state->client_pcb != NULL) {
        tcp_close(state->client_pcb);
        state->client_pcb = NULL;
    }
    state->complete = false;
    return ERR_OK;
}

// Send parsed data to the connected client
static err_t send_data_to_target(const char *direction, int speed) {
    if (!persistent_state || !persistent_state->client_pcb) {
        DEBUG_printf("No client connected, cannot send data\n");
        return ERR_CONN;
    }

    char send_buffer[BUF_SIZE];
    snprintf(send_buffer, sizeof(send_buffer), "Direction: %s; Speed: %d", direction, speed);

    DEBUG_printf("Sending telemetry to client: %s\n", send_buffer);

    err_t err = tcp_write(persistent_state->client_pcb, send_buffer, strlen(send_buffer), TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        DEBUG_printf("Failed to send data to client: %d\n", err);
        tcp_server_close(persistent_state);  // Close connection on error
        return err;
    }

    tcp_output(persistent_state->client_pcb);  // Ensure data is sent immediately
    return ERR_OK;
}

// Handle data reception from the client
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) {
        DEBUG_printf("Connection closed by client\n");
        tcp_server_close(arg);  // Close server connection if client disconnects
        return ERR_OK;
    }

    if (p->tot_len > 0) {
        char received_data[BUF_SIZE];

        // Copy and null-terminate the received payload
        memcpy(received_data, p->payload, p->tot_len);
        received_data[p->tot_len] = '\0';  // Null-terminate to make it a proper C string

        // Print the received content for debugging
        DEBUG_printf("Received data: %s\n", received_data);

        // Parse the direction and speed
        char movement[20] = {0};
        char turn_direction[20] = {0};
        int speed = 0;

        // Determine the movement direction
        if (strstr(received_data, "Forward")) {
            strcpy(movement, "Forward");
        } else if (strstr(received_data, "Backward")) {
            strcpy(movement, "Backward");
        } else if (strstr(received_data, "Stop Movement")) {
            strcpy(movement, "Stopped");
        }

        // Determine the turning direction
        if (strstr(received_data, "Left")) {
            strcpy(turn_direction, "Left");
        } else if (strstr(received_data, "Right")) {
            strcpy(turn_direction, "Right");
        } else if (strstr(received_data, "Stop Turning")) {
            strcpy(turn_direction, "Stopped");
        }

        // Extract speed from the received data if present
        if (sscanf(received_data, "%*[^0-9]%d", &speed) == 1) {
            telemetry_data.speed = speed;
        } else {
            telemetry_data.speed = 0;  // Default to 0 if speed is not found
        }

        // Formulate the final direction string based on conditions
        if (strcmp(movement, "Stopped") == 0 && strcmp(turn_direction, "Stopped") == 0) {
            // Only show "Stopped" if both movement and turning are stopped
            snprintf(telemetry_data.direction, sizeof(telemetry_data.direction), "Stopped");
        } else if (strcmp(turn_direction, "Stopped") == 0 || strcmp(turn_direction, "") == 0) {
            // Only show movement direction if there’s no turn or turn is stopped
            snprintf(telemetry_data.direction, sizeof(telemetry_data.direction), "%s", movement);
        } else if (strcmp(movement, "Stopped") == 0 || strcmp(movement, "") == 0) {
            // Only show turning direction if there’s no movement or movement is stopped
            snprintf(telemetry_data.direction, sizeof(telemetry_data.direction), "%s", turn_direction);
        } else {
            // Show both movement and turning directions if both are active
            snprintf(telemetry_data.direction, sizeof(telemetry_data.direction), "%s %s", movement, turn_direction);
        }

        DEBUG_printf("Parsed Direction: %s, Speed: %d\n", telemetry_data.direction, telemetry_data.speed);

        // Send parsed direction and speed to client
        send_data_to_target(telemetry_data.direction, telemetry_data.speed);

        tcp_recved(tpcb, p->tot_len);
    }
    pbuf_free(p);
    return ERR_OK;
}

// Handle client connection acceptance
static err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err) {
    if (err != ERR_OK || client_pcb == NULL) {
        DEBUG_printf("Failed to accept connection\n");
        tcp_server_close(arg);
        return ERR_VAL;
    }

    const char *client_ip = ipaddr_ntoa(&client_pcb->remote_ip);
    DEBUG_printf("Client connected from IP: %s\n", client_ip);

    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    state->client_pcb = client_pcb;
    persistent_state = state;  // Update persistent state for active connection

    tcp_arg(client_pcb, state);
    tcp_recv(client_pcb, tcp_server_recv);
    return ERR_OK;
}

// Open the TCP server and start listening
static bool tcp_server_open(TCP_SERVER_T *state) {
    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        DEBUG_printf("Failed to create pcb\n");
        return false;
    }

    err_t err = tcp_bind(pcb, IP_ADDR_ANY, TCP_PORT);
    if (err != ERR_OK) {
        DEBUG_printf("Failed to bind to port %u\n", TCP_PORT);
        tcp_close(pcb);
        return false;
    }

    state->server_pcb = tcp_listen_with_backlog(pcb, 1);
    if (!state->server_pcb) {
        DEBUG_printf("Failed to listen\n");
        tcp_close(pcb);
        return false;
    }

    tcp_arg(state->server_pcb, state);
    tcp_accept(state->server_pcb, tcp_server_accept);
    return true;
}

void run_tcp_server_test(void) {
    TCP_SERVER_T *state = tcp_server_init();
    if (!state) return;

    if (!tcp_server_open(state)) {
        tcp_server_close(state);
        free(state);
        return;
    }

    while (true) {
        #if PICO_CYW43_ARCH_POLL
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
        #else
        sleep_ms(1000);
        #endif
    }

    tcp_server_close(state);
    free(state);
}

int main() {
    stdio_init_all();

    DEBUG_printf("Searching for Wi-Fi...\n");
    if (cyw43_arch_init()) {
        DEBUG_printf("Failed to initialize Wi-Fi\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();

    DEBUG_printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        DEBUG_printf("Failed to connect to Wi-Fi\n");
        return 1;
    }
    DEBUG_printf("Connected to Wi-Fi\n");

    const ip4_addr_t *ip_addr = &netif_list->ip_addr;
    if (ip4_addr_isany_val(*ip_addr)) {
        DEBUG_printf("Failed to retrieve IP address\n");
    } else {
        DEBUG_printf("Pico W server IP address: %s\n", ip4addr_ntoa(ip_addr));
    }

    run_tcp_server_test();
    cyw43_arch_deinit();
    return 0;
}

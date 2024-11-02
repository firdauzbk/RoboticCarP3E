#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"  // Include the header for Wi-Fi support
#include "FreeRTOS.h"
#include "task.h"
#include "message_buffer.h"
#include "hardware/adc.h"

// Define the buffer size for storing messages
#define BUFFER_SIZE 1024
#define WIFI_CONNECT_TIMEOUT_MS 10000  // 10 seconds timeout for Wi-Fi connection

// Message buffer handle for communication between tasks (if needed)
static MessageBufferHandle_t xWiFiStatusBuffer;

// Function prototype for run_program to avoid implicit declaration warning
void run_program(void);

// Function to restart the program (soft reset)
void soft_reset() {
    printf("Button pressed, performing soft reset...\n");
    fflush(stdout);  // Ensure the message is sent

    // Small delay to ensure the message is seen before reset
    sleep_ms(500);

    // Call run_program() to simulate a restart
    run_program();  // Restart the main program logic
}

// Function to initialize Wi-Fi
bool init_wifi(void) {
    printf("Initializing Wi-Fi...\n");
    fflush(stdout);

    // Initialize the CYW43 driver (Wi-Fi/Bluetooth driver)
    if (cyw43_arch_init()) {
        printf("Failed to initialize Wi-Fi\n");
        return false;
    }

    // Attempt to connect to the Wi-Fi network using the provided SSID and password
    printf("Connecting to Wi-Fi...\n");
    fflush(stdout);
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, WIFI_CONNECT_TIMEOUT_MS)) {
        printf("Failed to connect to Wi-Fi. Please check your credentials or network.\n");
        cyw43_arch_deinit();  // Deinitialize if connection failed
        return false;
    }

    printf("Connected to Wi-Fi\n");
    fflush(stdout);
    return true;
}

// Task for handling Wi-Fi connection
void wifi_task(__unused void *params) {
    // Attempt to initialize and connect to Wi-Fi
    if (!init_wifi()) {
        printf("Wi-Fi initialization failed. Restarting...\n");
        fflush(stdout);
        soft_reset();  // Retry if initialization fails
        vTaskDelete(NULL);  // Delete the task if failed
        return;
    }

    // Send Wi-Fi status to the message buffer if needed
    if (xWiFiStatusBuffer != NULL) {
        const char *status_message = "Wi-Fi Connected";
        xMessageBufferSend(xWiFiStatusBuffer, status_message, strlen(status_message), 0);
    }

    // Optional: Perform additional Wi-Fi related tasks here
    while (true) {
        // You can add code here to periodically check the connection or perform network tasks
        vTaskDelay(pdMS_TO_TICKS(1000));  // Delay for 1 second
    }

    // Disconnect Wi-Fi before exiting (not usually needed in infinite loop)
    cyw43_arch_deinit();
}

// Function containing the main program logic
void run_program(void) {
    // Configure GP20 and GP21 as input with pull-up resistors
    const uint BUTTON_PIN_RESET = 20;
    const uint BUTTON_PIN_DISPLAY = 21;
    gpio_init(BUTTON_PIN_RESET);
    gpio_set_dir(BUTTON_PIN_RESET, GPIO_IN);
    gpio_pull_up(BUTTON_PIN_RESET);
    gpio_init(BUTTON_PIN_DISPLAY);
    gpio_set_dir(BUTTON_PIN_DISPLAY, GPIO_IN);
    gpio_pull_up(BUTTON_PIN_DISPLAY);

    // Create the message buffer for Wi-Fi status (if needed)
    xWiFiStatusBuffer = xMessageBufferCreate(BUFFER_SIZE);

    // Create the Wi-Fi connection task
    xTaskCreate(wifi_task, "WiFi Task", 1024, NULL, tskIDLE_PRIORITY + 1, NULL);

    // Delay to ensure serial connection is ready
    sleep_ms(5000);  // Wait for 5 seconds to give the serial monitor time to connect

    // Print the initial prompt when the program starts
    printf("Pico is ready to receive data... \n");
    fflush(stdout);  // Ensure the message is sent immediately

    char message_buffer[BUFFER_SIZE];  // Buffer to store received messages
    int buffer_index = 0;  // Current index in the buffer
    bool prompt_needed = false;  // Flag to control when to show the "ready" message

    // Infinite loop to continuously check for incoming data and button press
    while (true) {
        // Check if the reset button is pressed (GP20 is low)
        if (gpio_get(BUTTON_PIN_RESET) == 0) {
            // Debounce delay: wait for 50ms to confirm the button press
            sleep_ms(50);
            if (gpio_get(BUTTON_PIN_RESET) == 0) {  // Check again if still pressed
                // Perform a soft reset by calling the reset function
                soft_reset();
                return;  // Exit the current function after the reset
            }
        }

        // Check if the display button is pressed (GP21 is low)
        if (gpio_get(BUTTON_PIN_DISPLAY) == 0) {
            // Debounce delay: wait for 50ms to confirm the button press
            sleep_ms(50);
            if (gpio_get(BUTTON_PIN_DISPLAY) == 0) {  // Check again if still pressed
                // Print the contents of the message buffer line by line
                printf("Stored Messages:\n");
                if (buffer_index > 0) {
                    int start = 0;  // Start of the current line
                    // Go through the buffer and print each line
                    for (int i = 0; i < buffer_index; i++) {
                        if (message_buffer[i] == '\n') {
                            // Print the current line from start to i
                            printf("%.*s\n", i - start, &message_buffer[start]);
                            start = i + 1;  // Move to the start of the next line
                        }
                    }
                    // Print any remaining characters if the last line didn't end with '\n'
                    if (start < buffer_index) {
                        printf("%.*s\n", buffer_index - start, &message_buffer[start]);
                    }
                } else {
                    printf("No messages stored.\n");
                }
                fflush(stdout);  // Ensure the message is sent
            }
        }

        // Read one byte from the UART with a 500ms timeout
        int ch = getchar_timeout_us(500000);  // 500ms timeout for checking data

        // Check if data was received (not a timeout)
        if (ch != PICO_ERROR_TIMEOUT) {
            // Store the received character in the buffer if there is space
            if (buffer_index < BUFFER_SIZE - 1) {
                message_buffer[buffer_index++] = (char)ch;
                message_buffer[buffer_index] = '\0';  // Null-terminate the buffer
            }

            // Print the received character
            printf("Received byte: %c\n", (char)ch);
            fflush(stdout);  // Ensure the character is printed immediately
            prompt_needed = true;  // Set flag to print the prompt after processing input
        } else if (prompt_needed) {
            // Print the prompt again once data has been processed
            printf("Pico is ready to receive data... \n");
            fflush(stdout);  // Ensure the message is sent immediately
            prompt_needed = false;  // Reset the flag to prevent repeated prompts
        }

        // Optional: small delay to reduce loop frequency
        sleep_ms(100);
    }
}

int main(void) {
    // Initialize standard I/O over USB
    stdio_init_all();
    sleep_ms(2000);  // Delay to ensure the serial monitor is ready

    // Print a starting message
    printf("Starting program...\n");
    fflush(stdout);

    // Start the FreeRTOS scheduler
    vTaskStartScheduler();

    // If the scheduler exits, start the main program logic (shouldn't happen)
    printf("Scheduler exited, starting run_program...\n");
    fflush(stdout);
    run_program();

    // Return 0 to indicate that the program has ended (in case it ever does)
    return 0;
}

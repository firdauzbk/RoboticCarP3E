/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <stdio.h>

#define TEMP_TASK_PRIORITY         ( tskIDLE_PRIORITY + 2 )
#define MOVING_AVG_TASK_PRIORITY   ( tskIDLE_PRIORITY + 1 )
#define SIMPLE_AVG_TASK_PRIORITY   ( tskIDLE_PRIORITY + 1 )
#define PRINT_TASK_PRIORITY        ( tskIDLE_PRIORITY + 3 )

#define TEMP_READ_PERIOD_MS        1000   // Read temperature every 1 second
#define BUFFER_SIZE                10     // Size of the buffer for moving average

// Task handles
TaskHandle_t tempTaskHandle = NULL;
TaskHandle_t movingAvgTaskHandle = NULL;
TaskHandle_t simpleAvgTaskHandle = NULL;
TaskHandle_t printTaskHandle = NULL;

// Queues for inter-task communication
QueueHandle_t tempQueue;        // Queue to send temperature readings
QueueHandle_t printQueue;       // Queue to send messages to the print task

// Mutex for synchronized printf
SemaphoreHandle_t printMutex;

// Function to read temperature from RP2040's built-in sensor
float read_temperature() {
    // Select the temperature sensor channel
    adc_select_input(4);

    // Read raw value from the ADC
    uint16_t raw = adc_read();

    // Convert raw value to voltage
    const float conversion_factor = 3.3f / (1 << 12);  // 12-bit resolution (0-4095)
    float voltage = raw * conversion_factor;

    // Convert voltage to temperature (formula from datasheet)
    float temperature = 27.0f - (voltage - 0.706f) / 0.001721f;

    return temperature;
}

// Task to handle all printf operations
void print_task(void *params) {
    char printBuffer[128];
    while (true) {
        if (xQueueReceive(printQueue, &printBuffer, portMAX_DELAY) == pdPASS) {
            // Only print inside this task to adhere to the requirements
            xSemaphoreTake(printMutex, portMAX_DELAY);
            printf("%s", printBuffer);
            xSemaphoreGive(printMutex);
        }
    }
}

// Task to read the temperature and send it to other tasks
void temp_task(void *params) {
    float tempReading;
    while (true) {
        // Read temperature from sensor
        tempReading = read_temperature();
        
        // Send the reading to the moving average and simple average tasks
        xQueueSend(tempQueue, &tempReading, portMAX_DELAY);
        
        // Prepare message to print
        char msg[50];
        snprintf(msg, sizeof(msg), "Temperature Reading: %.2f°C\n", tempReading);
        xQueueSend(printQueue, &msg, portMAX_DELAY);
        
        // Wait for the specified period
        vTaskDelay(pdMS_TO_TICKS(TEMP_READ_PERIOD_MS));
    }
}

// Task to calculate moving average of the last 10 temperature readings
void moving_avg_task(void *params) {
    float buffer[BUFFER_SIZE] = {0};  // Circular buffer for readings
    int index = 0;
    int count = 0;  // Number of readings in buffer
    float sum = 0;

    float newReading;

    while (true) {
        // Wait for a new temperature reading
        if (xQueueReceive(tempQueue, &newReading, portMAX_DELAY) == pdPASS) {
            // Subtract the old value and add the new value
            sum -= buffer[index];
            buffer[index] = newReading;
            sum += newReading;

            // Increment index and wrap around if necessary
            index = (index + 1) % BUFFER_SIZE;

            // Calculate the moving average
            if (count < BUFFER_SIZE) {
                count++;
            }
            float movingAverage = sum / count;

            // Prepare message to print
            char msg[50];
            snprintf(msg, sizeof(msg), "Moving Average: %.2f°C\n", movingAverage);
            xQueueSend(printQueue, &msg, portMAX_DELAY);
        }
    }
}

// Task to calculate simple average of all temperature readings received
void simple_avg_task(void *params) {
    float totalSum = 0;
    int totalCount = 0;
    float newReading;

    while (true) {
        // Wait for a new temperature reading
        if (xQueueReceive(tempQueue, &newReading, portMAX_DELAY) == pdPASS) {
            // Update the total sum and count
            totalSum += newReading;
            totalCount++;

            // Calculate the simple average
            float simpleAverage = totalSum / totalCount;

            // Prepare message to print
            char msg[50];
            snprintf(msg, sizeof(msg), "Simple Average: %.2f°C\n", simpleAverage);
            xQueueSend(printQueue, &msg, portMAX_DELAY);
        }
    }
}

int main(void) {
    stdio_init_all();
    
    // Initialize ADC hardware
    adc_init();
    adc_set_temp_sensor_enabled(true);

    // Create queues
    tempQueue = xQueueCreate(10, sizeof(float));   // Queue for temperature readings
    printQueue = xQueueCreate(10, sizeof(char[128])); // Queue for messages to print task
    
    // Create mutex for printf synchronization
    printMutex = xSemaphoreCreateMutex();

    // Create the tasks
    xTaskCreate(temp_task, "TempTask", 256, NULL, TEMP_TASK_PRIORITY, &tempTaskHandle);
    xTaskCreate(moving_avg_task, "MovingAvgTask", 256, NULL, MOVING_AVG_TASK_PRIORITY, &movingAvgTaskHandle);
    xTaskCreate(simple_avg_task, "SimpleAvgTask", 256, NULL, SIMPLE_AVG_TASK_PRIORITY, &simpleAvgTaskHandle);
    xTaskCreate(print_task, "PrintTask", 256, NULL, PRINT_TASK_PRIORITY, &printTaskHandle);

    // Start the FreeRTOS scheduler
    vTaskStartScheduler();

    // Should never reach here
    while (true) {}
    
    return 0;
}

// #ifndef BUDDY3_H
// #define BUDDY3_H

// #include <stdint.h>
// #include <stdbool.h>
// #include <stdio.h>
// #include "pico/stdlib.h"
// #include <string.h> 
// #include "hardware/adc.h"
// #include "hardware/gpio.h"
// #include "buddy3.h"
// #include "buddy2.h"

// #define LEFT_IR_SENSOR_ANALOG_PIN 26   // ADC GPIO pin for the left sensor
// #define RIGHT_IR_SENSOR_ANALOG_PIN 27   // ADC GPIO pin for the right sensor

// // Fixed threshold for surface detection
// extern const int BLACK_WHITE_THRESHOLD;

// // Character and binary code arrays
// extern char array_char[];
// extern char *array_code[];
// extern char *array_reverse_code[];

// // Function prototypes
// void setup_adc();                               // Set up ADC for IR sensors
// uint16_t read_adc(int sensor_index);           // Read analog value from specified sensor
// void read_ir_sensors();                         // Read and process IR sensor data
// void line_following(uint16_t analog_values[]); // Control line following based on sensor readings
// void print_detected_state(int sensor_index, bool current_state_black, uint16_t analog_value, float voltage); // Print current sensor state
// int find_binary_index(const char *binary_code, char *code_array[]);
// char map_binary_to_char(const char *binary_code, bool reverse);
// void barcode_detector(uint16_t analog_value);
// void convert_stay_counts(int stay_counts[], int barcount);
// #endif // BUDDY3_H

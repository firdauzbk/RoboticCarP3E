#ifndef BUDDY3_H
#define BUDDY3_H

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"

// Define ADC GPIO pins for the sensors
#define LEFT_IR_SENSOR_ANALOG_PIN 26
#define RIGHT_IR_SENSOR_ANALOG_PIN 27

// Variables to track turn direction and attempt count
extern int turn_attempts;
extern bool turn_left;

// Fixed threshold for surface detection
extern const int BLACK_WHITE_THRESHOLD;

// Timing variables for sensors
extern uint64_t before_time;
extern uint64_t time_on_black[2];
extern uint64_t time_on_white[2];
extern bool last_state_black[2];

// Function prototypes
void setup_adc();
uint16_t read_adc(int sensor_index);
void line_following(uint16_t analog_values[]);
void print_surface_time(int sensor_index);
void process_ir_reading(uint16_t analog_value, int sensor_index, float voltage);
void read_ir_sensors();
void print_detected_state(int sensor_index, bool current_state_black, uint16_t analog_value, float voltage);

#endif // BUDDY3_H
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "buddy3.h"

#define LEFT_IR_SENSOR_ANALOG_PIN 26   // ADC GPIO pin for the left sensor
#define RIGHT_IR_SENSOR_ANALOG_PIN 27   // ADC GPIO pin for the right sensor

// Variables to track turn direction and attempt count
int turn_attempts = 0;
bool turn_left = true;  // Start with small left turns when off the line

// Fixed threshold for surface detection
const int BLACK_WHITE_THRESHOLD = 200;

// Timing variables for sensors
uint64_t before_time = 0;
uint64_t time_on_black[2] = {0, 0};  // Time on black for left and right
uint64_t time_on_white[2] = {0, 0};  // Time on white for left and right
bool last_state_black[2] = {false, false}; // Last states for left and right

// Function prototypes
void setup_adc();
uint16_t read_adc(int sensor_index);
void line_following(uint16_t analog_values[]);
void print_surface_time(int sensor_index);
void process_ir_reading(uint16_t analog_value, int sensor_index, float voltage);
void read_ir_sensors();
void print_detected_state(int sensor_index, bool current_state_black, uint16_t analog_value, float voltage);

// Function to set up ADC
void setup_adc() {
    adc_init();
    adc_gpio_init(LEFT_IR_SENSOR_ANALOG_PIN);
    adc_gpio_init(RIGHT_IR_SENSOR_ANALOG_PIN);
}

// Function to select ADC input based on sensor index
uint16_t read_adc(int sensor_index) {
    adc_select_input(sensor_index); // 0 for left, 1 for right
    return adc_read(); // Read the selected ADC input
}

// Function to read IR sensors and print current state and values
void read_ir_sensors() {
    uint16_t analog_values[2];
    float voltages[2];

    // Read analog values from the sensors
    for (int i = 0; i < 2; i++) {
        analog_values[i] = read_adc(i); // Read from the corresponding sensor
        voltages[i] = analog_values[i] * (3.3f / 4095.0f);  // Convert analog value to voltage
        process_ir_reading(analog_values[i], i, voltages[i]);
    }

    // Call the line following function with the analog values
    line_following(analog_values);
}

void process_ir_reading(uint16_t analog_value, int sensor_index, float voltage) {
    // Determine surface contrast based on the fixed threshold 
    bool current_state_black = (analog_value > BLACK_WHITE_THRESHOLD);

    // Calculate elapsed time since the last state change
    uint64_t current_time = time_us_64();
    uint64_t elapsed_time = current_time - before_time;
    before_time = current_time; // Update last time

    // Measure time spent in the current state
    if (current_state_black) {
        time_on_black[sensor_index] += elapsed_time;  // Accumulate time spent on black surface
        // If transitioning to black, print time on white
        if (!last_state_black[sensor_index]) {
            print_surface_time(sensor_index);
            time_on_white[sensor_index] = 0; // Reset time on white surface
        }
    } else {
        time_on_white[sensor_index] += elapsed_time;  // Accumulate time spent on white surface
        // If transitioning to white, print time on black
        if (last_state_black[sensor_index]) {
            print_surface_time(sensor_index);
            time_on_black[sensor_index] = 0; // Reset time on black surface
        }
    }

    // Update last state
    last_state_black[sensor_index] = current_state_black;

    // Print detected state and values
    print_detected_state(sensor_index, current_state_black, analog_value, voltage);
}

void print_surface_time(int sensor_index) {
    if (last_state_black[sensor_index]) {
        printf("Sensor %d - Time on Black Surface: %.6f seconds\n", sensor_index, time_on_black[sensor_index] / 1e6); // Convert to seconds
    } else {
        printf("Sensor %d - Time on White Surface: %.6f seconds\n", sensor_index, time_on_white[sensor_index] / 1e6); // Convert to seconds
    }
}

void print_detected_state(int sensor_index, bool current_state_black, uint16_t analog_value, float voltage) {
    if (current_state_black) {
        printf("Sensor %d - Detected: Black Surface\n", sensor_index);
    } else {
        printf("Sensor %d - Detected: White Surface\n", sensor_index);
    }

    // Print the raw analog value and voltage for debugging
    printf("Sensor %d - Analog Value: %u, Voltage: %.2fV\n", sensor_index, analog_value, voltage);
}

// Function to control line following based on right sensor

void line_following(uint16_t analog_value[]) {
    bool on_black = (analog_value > BLACK_WHITE_THRESHOLD);

    if (on_black) {
        // Reset turn direction and attempts on detecting black
        turn_attempts = 0;
        turn_left = true;
        printf("On black surface. Moving forward...\n");

    } else {
        // Detect white surface: initiate turn sequence
        if (turn_left) {
            printf("On white surface. Small left turn...\n");
            // Implement your small left turn code here (e.g., adjust motor speeds)

            turn_attempts++;
            // After 5 small turns to the left, switch to a larger right turn
            if (turn_attempts >= 5) {
                turn_left = false;
            }
        } else {
            printf("On white surface. Large right turn...\n");
            // Implement your large right turn code here (e.g., rotate 90 degrees)

            // Reset turn attempts after a right turn and try small left turns again
            turn_attempts = 0;
            turn_left = true;
        }
    }
}
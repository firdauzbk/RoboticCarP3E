#include "pico/stdlib.h"
#include <stdio.h>
#include <stdbool.h>
#include "hardware/gpio.h"
#include "../buddy2/buddy2.h"
#include "hardware/timer.h"
#include "buddy5.h"
#include <math.h> // for M_PI

// Constants for the wheel and encoder
const float WHEEL_DIAMETER_CM = 2.5;
const float WHEEL_CIRCUMFERENCE_CM = M_PI * WHEEL_DIAMETER_CM;
const unsigned int SLOTS_PER_REVOLUTION = 20;
const float DISTANCE_PER_PULSE_CM = WHEEL_CIRCUMFERENCE_CM / SLOTS_PER_REVOLUTION;

// Variables for left and right IR encoder tracking
volatile int left_pulse_count = 0;
volatile float left_incremental_distance = 0;
volatile float left_total_distance = 0;
volatile float left_speed_cm_s = 0;
volatile uint64_t left_last_pulse_time = 0;

volatile int right_pulse_count = 0;
volatile float right_incremental_distance = 0;
volatile float right_total_distance = 0;
volatile float right_speed_cm_s = 0;
volatile uint64_t right_last_pulse_time = 0;

uint64_t last_distance_check_time = 0;

// Initialization function for ultrasonic, encoder, and buzzer
void initializeBuddy5Components() {
    stdio_init_all();
    setupEncoderPins();
    last_distance_check_time = time_us_64();
}

// Interrupt callback for encoder to calculate speed and distance
void encoder_callback(uint gpio, uint32_t events) {
    uint64_t current_time = time_us_64(); // Get the current time in microseconds

    if (gpio == LEFT_ENCODER_PIN) {
        left_pulse_count++;
        left_incremental_distance += DISTANCE_PER_PULSE_CM;
        left_total_distance += DISTANCE_PER_PULSE_CM;

        if (left_last_pulse_time != 0) { // Calculate speed if not the first pulse
            float time_diff_s = (current_time - left_last_pulse_time) / 1e6; // Convert to seconds
            left_speed_cm_s = DISTANCE_PER_PULSE_CM / time_diff_s;
        }

        left_last_pulse_time = current_time;

    } else if (gpio == RIGHT_ENCODER_PIN) {
        right_pulse_count++;
        right_incremental_distance += DISTANCE_PER_PULSE_CM;
        right_total_distance += DISTANCE_PER_PULSE_CM;

        if (right_last_pulse_time != 0) { // Calculate speed if not the first pulse
            float time_diff_s = (current_time - right_last_pulse_time) / 1e6; // Convert to seconds
            right_speed_cm_s = DISTANCE_PER_PULSE_CM / time_diff_s;
        }

        right_last_pulse_time = current_time;
    }
}

// Encoder setup function
void setupEncoderPins() {
    gpio_init(LEFT_ENCODER_PIN);
    gpio_set_dir(LEFT_ENCODER_PIN, GPIO_IN);
    gpio_set_irq_enabled_with_callback(LEFT_ENCODER_PIN, GPIO_IRQ_EDGE_RISE, true, &encoder_callback);

    gpio_init(RIGHT_ENCODER_PIN);
    gpio_set_dir(RIGHT_ENCODER_PIN, GPIO_IN);
    gpio_set_irq_enabled_with_callback(RIGHT_ENCODER_PIN, GPIO_IRQ_EDGE_RISE, true, &encoder_callback);
}

void right_encoder_callback(uint gpio, uint32_t events) {
    right_pulse_count++;
    right_incremental_distance += DISTANCE_PER_PULSE_CM;

    if (right_last_pulse_time != 0) {
        uint64_t current_time = time_us_64();
        float time_diff_s = (current_time - right_last_pulse_time) / 1e6;
        right_speed_cm_s = DISTANCE_PER_PULSE_CM / time_diff_s;

        // Debugging output
        printf("Right Encoder Pulse Count: %d, Right Motor Speed: %.2f cm/s\n", right_pulse_count, right_speed_cm_s);

        right_last_pulse_time = current_time;
    }
}

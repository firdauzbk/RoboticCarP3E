#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "buddy5.h"
#include <math.h> // for M_PI

// Constants for the ultrasonic sensor and buzzer
#define BUZZER_PIN 18

// Constants for the wheel and encoder
const float WHEEL_DIAMETER_CM = 2.5;
const float WHEEL_CIRCUMFERENCE_CM = M_PI * WHEEL_DIAMETER_CM;
const unsigned int SLOTS_PER_REVOLUTION = 20;
const float DISTANCE_PER_PULSE_CM = WHEEL_CIRCUMFERENCE_CM / SLOTS_PER_REVOLUTION;

// Variables for Left and Right IR encoders
volatile int left_pulse_count = 0;
volatile float left_incremental_distance = 0;
volatile float left_total_distance = 0;

volatile int right_pulse_count = 0;
volatile float right_incremental_distance = 0;
volatile float right_total_distance = 0;

// Ultrasonic sensor pins (GP4 and GP5)
const unsigned int TRIG_PIN = 5;
const unsigned int ECHO_PIN = 4;

// Encoder pins for Left (GP0 and GP1) and Right (GP2 and GP3)
const unsigned int LEFT_ENCODER_PIN_A = 0;
const unsigned int LEFT_ENCODER_PIN_B = 1;
const unsigned int RIGHT_ENCODER_PIN_A = 8;
const unsigned int RIGHT_ENCODER_PIN_B = 9;

uint64_t last_distance_check_time = 0;  // Track the last check time

// Initialization function for ultrasonic, encoder, and buzzer
void initializeBuddy5Components() {
    stdio_init_all();  
    setupUltrasonicPins();
    setupEncoderPins();
    setupBuzzerPin();
    last_distance_check_time = time_us_64();  // Initialize the last distance check time
}

// Function to check distance from the ultrasonic sensor and activate the buzzer if object is close
void measureDistanceAndBuzz() {
    uint64_t current_time = time_us_64();

    // Check if enough time has passed since the last check
    if (current_time - last_distance_check_time > CHECK_INTERVAL_MS * 1000) {
        float distanceCm = getCm();

        if (distanceCm == 0.0) {
            //printf("No echo detected within range.\n");
        } else {
            printf("Distance measured: %.2f cm\n", distanceCm);

            if (distanceCm < DISTANCE_THRESHOLD_CM) {
                //printf("Object too close! Buzzing...\n");
                gpio_put(BUZZER_PIN, 1);
                sleep_ms(200);  // Buzz duration
                gpio_put(BUZZER_PIN, 0);
            }
        }

        // Update the last check time
        last_distance_check_time = current_time;
    }
}

// Helper function to set up ultrasonic sensor pins
void setupUltrasonicPins() {
    gpio_init(TRIG_PIN);
    gpio_init(ECHO_PIN);
    gpio_set_dir(TRIG_PIN, GPIO_OUT);
    gpio_set_dir(ECHO_PIN, GPIO_IN);
}

// Helper function to set up buzzer pin
void setupBuzzerPin() {
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
}

// Helper function to get pulse duration for distance measurement
uint64_t getPulse() {
    gpio_put(TRIG_PIN, 0);
    sleep_us(2);
    gpio_put(TRIG_PIN, 1);
    sleep_us(10);
    gpio_put(TRIG_PIN, 0);

    uint64_t startWait = time_us_64();
    while (gpio_get(ECHO_PIN) == 0) {
        if (time_us_64() - startWait > 30000) {
            //printf("Timeout: No echo detected\n");
            return 0;
        }
    }

    uint64_t pulseStart = time_us_64();
    while (gpio_get(ECHO_PIN) == 1) {
        if (time_us_64() - pulseStart > 30000) {
            //printf("Timeout: Echo duration exceeded\n");
            return 0;
        }
    }
    uint64_t pulseEnd = time_us_64();

    //printf("Echo pulse duration: %llu us\n", pulseEnd - pulseStart);
    return pulseEnd - pulseStart;
}

// Helper function to calculate distance in cm
float getCm() {
    uint64_t pulseLength = getPulse();
    if (pulseLength == 0) return 0.0;

    float distance = (float)pulseLength / 29.0 / 2.0;
    printf("Calculated distance: %.2f cm\n", distance);
    return distance;
}

// Interrupt callback for left encoder
void left_encoder_callback(uint gpio, uint32_t events) {
    left_pulse_count++;
    left_incremental_distance += DISTANCE_PER_PULSE_CM;

    if (left_pulse_count >= SLOTS_PER_REVOLUTION) {
        left_total_distance += left_incremental_distance;
        left_incremental_distance = 0;
        left_pulse_count = 0;

        printf("Left Wheel - Full revolution completed. Total distance: %.2f cm\n", left_total_distance);
    } else {
        printf("Left Wheel - Current pulse count: %d, Incremental distance: %.2f cm, Total distance: %.2f cm\n", 
               left_pulse_count, left_incremental_distance, left_total_distance + left_incremental_distance);
    }
}

// Interrupt callback for right encoder
void right_encoder_callback(uint gpio, uint32_t events) {
    right_pulse_count++;
    right_incremental_distance += DISTANCE_PER_PULSE_CM;

    if (right_pulse_count >= SLOTS_PER_REVOLUTION) {
        right_total_distance += right_incremental_distance;
        right_incremental_distance = 0;
        right_pulse_count = 0;

        printf("Right Wheel - Full revolution completed. Total distance: %.2f cm\n", right_total_distance);
    } else {
        printf("Right Wheel - Current pulse count: %d, Incremental distance: %.2f cm, Total distance: %.2f cm\n", 
               right_pulse_count, right_incremental_distance, right_total_distance + right_incremental_distance);
    }
}

// Function to set up encoder pins and interrupts
void setupEncoderPins() {
    // Set up pins for the left encoder
    gpio_init(LEFT_ENCODER_PIN_A);
    gpio_init(LEFT_ENCODER_PIN_B);
    gpio_set_dir(LEFT_ENCODER_PIN_A, GPIO_IN);
    gpio_set_dir(LEFT_ENCODER_PIN_B, GPIO_IN);
    gpio_set_irq_enabled_with_callback(LEFT_ENCODER_PIN_A, GPIO_IRQ_EDGE_RISE, true, &left_encoder_callback);
    gpio_set_irq_enabled_with_callback(LEFT_ENCODER_PIN_B, GPIO_IRQ_EDGE_RISE, true, &left_encoder_callback);

    // Set up pins for the right encoder
    gpio_init(RIGHT_ENCODER_PIN_A);
    gpio_init(RIGHT_ENCODER_PIN_B);
    gpio_set_dir(RIGHT_ENCODER_PIN_A, GPIO_IN);
    gpio_set_dir(RIGHT_ENCODER_PIN_B, GPIO_IN);
    gpio_set_irq_enabled_with_callback(RIGHT_ENCODER_PIN_A, GPIO_IRQ_EDGE_RISE, true, &right_encoder_callback);
    gpio_set_irq_enabled_with_callback(RIGHT_ENCODER_PIN_B, GPIO_IRQ_EDGE_RISE, true, &right_encoder_callback);
}

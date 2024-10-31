#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "buddy5.h"
#include <math.h> // for M_PI

// Constants for the ultrasonic sensor and buzzer
#define BUZZER_PIN 18
#define DISTANCE_THRESHOLD_CM 10.0
#define CHECK_INTERVAL_MS 200

// Constants for the wheel and encoder
const float WHEEL_DIAMETER_CM = 2.5;
const float WHEEL_CIRCUMFERENCE_CM = M_PI * WHEEL_DIAMETER_CM;
const unsigned int SLOTS_PER_REVOLUTION = 20;
const float DISTANCE_PER_PULSE_CM = WHEEL_CIRCUMFERENCE_CM / SLOTS_PER_REVOLUTION;

// Variables for IR encoder
volatile int pulse_count = 0;
volatile float incremental_distance = 0;
volatile float total_distance = 0;
volatile float last_distance_traveled = 0;

// Ultrasonic sensor pins (GP0 and GP1)
const unsigned int TRIG_PIN = 1;
const unsigned int ECHO_PIN = 0;

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
            printf("No echo detected within range.\n");
        } else {
            printf("Distance measured: %.2f cm\n", distanceCm);

            if (distanceCm < DISTANCE_THRESHOLD_CM) {
                printf("Object too close! Buzzing...\n");
                gpio_put(BUZZER_PIN, 1);
                sleep_ms(200);  // Buzz duration
                gpio_put(BUZZER_PIN, 0);
            }
        }

        // Update the last check time
        last_distance_check_time = current_time;
    }
}

// Function to manually update the last check time if needed
void updateLastCheckTime() {
    last_distance_check_time = time_us_64();
}

// Helper function to set up ultrasonic sensor pins
void setupUltrasonicPins()
{
    gpio_init(TRIG_PIN);
    gpio_init(ECHO_PIN);
    gpio_set_dir(TRIG_PIN, GPIO_OUT);
    gpio_set_dir(ECHO_PIN, GPIO_IN);
}

// Helper function to set up buzzer pin
void setupBuzzerPin()
{
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
}

// Helper function to get pulse duration for distance measurement
uint64_t getPulse()
{
    gpio_put(TRIG_PIN, 0);
    sleep_us(2);
    gpio_put(TRIG_PIN, 1);
    sleep_us(10);
    gpio_put(TRIG_PIN, 0);

    uint64_t startWait = time_us_64();
    while (gpio_get(ECHO_PIN) == 0) {
        if (time_us_64() - startWait > 30000) {
            printf("Timeout: No echo detected\n");
            return 0;
        }
    }

    uint64_t pulseStart = time_us_64();
    while (gpio_get(ECHO_PIN) == 1) {
        if (time_us_64() - pulseStart > 30000) {
            printf("Timeout: Echo duration exceeded\n");
            return 0;
        }
    }
    uint64_t pulseEnd = time_us_64();

    printf("Echo pulse duration: %llu us\n", pulseEnd - pulseStart);
    return pulseEnd - pulseStart;
}

// Helper function to calculate distance in cm
float getCm()
{
    uint64_t pulseLength = getPulse();
    if (pulseLength == 0) return 0.0;

    float distance = (float)pulseLength / 29.0 / 2.0;
    printf("Calculated distance: %.2f cm\n", distance);
    return distance;
}

// Interrupt callback for encoder
void encoder_callback(uint gpio, uint32_t events)
{
    pulse_count++;
    incremental_distance += DISTANCE_PER_PULSE_CM;

    if (pulse_count >= SLOTS_PER_REVOLUTION) {
        last_distance_traveled = incremental_distance;
        total_distance += incremental_distance;
        incremental_distance = 0;
        pulse_count = 0;

        printf("Full revolution completed. Last distance traveled: %.2f cm, Total distance: %.2f cm\n", 
               last_distance_traveled, total_distance);
    } else {
        last_distance_traveled = incremental_distance;
        printf("Partial revolution: Current pulse count: %d, Last distance traveled: %.2f cm, Total distance: %.2f cm\n", 
               pulse_count, last_distance_traveled, total_distance + incremental_distance);
    }
}

// Function to set up encoder pins and interrupts
void setupEncoderPins()
{
    gpio_init(2);
    gpio_init(3);
    gpio_set_dir(2, GPIO_IN);
    gpio_set_dir(3, GPIO_IN);

    gpio_set_irq_enabled_with_callback(2, GPIO_IRQ_EDGE_RISE, true, &encoder_callback);
    gpio_set_irq_enabled_with_callback(3, GPIO_IRQ_EDGE_RISE, true, &encoder_callback);
}

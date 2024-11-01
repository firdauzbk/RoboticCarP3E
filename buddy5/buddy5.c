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

// Ultrasonic sensor pins (GP4 and GP5)
const unsigned int TRIG_PIN = 5;
const unsigned int ECHO_PIN = 4;

uint64_t last_distance_check_time = 0;

// Initialization function for ultrasonic, encoder, and buzzer
void initializeBuddy5Components() {
    stdio_init_all();  
    setupUltrasonicPins();
    setupEncoderPins();
    setupBuzzerPin();
    last_distance_check_time = time_us_64();  
}

// Function to check distance from the ultrasonic sensor and activate the buzzer if object is close
void measureDistanceAndBuzz() {
    uint64_t current_time = time_us_64();
    if (current_time - last_distance_check_time > CHECK_INTERVAL_MS * 1000) {
        float distanceCm = getCm();
        if (distanceCm != 0.0 && distanceCm < DISTANCE_THRESHOLD_CM) {
            gpio_put(BUZZER_PIN, 1);
            sleep_ms(200);  
            gpio_put(BUZZER_PIN, 0);
        }
        last_distance_check_time = current_time;
    }
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
float getCm() {
    uint64_t pulseLength = getPulse();
    if (pulseLength == 0) return 0.0;

    float distance = (float)pulseLength / 29.0 / 2.0;
    printf("Calculated distance: %.2f cm\n", distance);
    return distance;
}

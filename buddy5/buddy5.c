#include "pico/stdlib.h"
#include <stdio.h>
#include <stdbool.h>
#include "hardware/gpio.h"
#include "../buddy2/buddy2.h"
#include "hardware/timer.h"
#include "buddy5.h"
#include <math.h> // for M_PI
#include <stdlib.h>

// Constants for measurement limits and filtering
#define MAX_DISTANCE_CM 400.0
#define MIN_DISTANCE_CM 2.0
#define SPEED_OF_SOUND_CM_US 0.0343
#define MEASUREMENT_TIMEOUT_US 25000

// Constants for the ultrasonic sensor and buzzer
#define BUZZER_PIN 18
#define DISTANCE_THRESHOLD_CM 15.0
#define CHECK_INTERVAL_MS 200

// Constants for the wheel and encoder
const float WHEEL_DIAMETER_CM = 6.6;
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
const unsigned int TRIG_PIN = 4;
const unsigned int ECHO_PIN = 5;

// Kalman filter variables
static kalman_state *distance_filter = NULL;
volatile absolute_time_t start_time;
volatile uint64_t pulse_width = 0;
volatile bool measurement_valid = false;

uint64_t last_distance_check_time = 0;

// Obstacle detection and buzzer control
volatile bool obstacle_detected = false;
volatile bool buzzer_on = false;
uint64_t buzzer_start_time = 0;

// Kalman filter functions
kalman_state *kalman_init(double q, double r, double p, double initial_value) {
    kalman_state *state = calloc(1, sizeof(kalman_state));
    if (state == NULL) {
        return NULL;
    }
    
    state->q = q > 0 ? q : 1.0;
    state->r = r > 0 ? r : 0.5;
    state->p = p > 0 ? p : 1.0;
    state->x = initial_value;
    return state;
}

void kalman_update(kalman_state *state, double measurement) {
    if (state == NULL || measurement < MIN_DISTANCE_CM || measurement > MAX_DISTANCE_CM) {
        return;
    }

    double innovation = measurement - state->x;
    
    if (fabs(innovation) > 10.0) {
        state->q *= 2.0;
    } else {
        state->q = 1.0;
    }

    state->p = state->p + state->q;
    state->k = state->p / (state->p + state->r);
    
    if (fabs(innovation) < 50.0) {
        state->x += state->k * innovation;
    } else {
        state->x = 0.7 * measurement + 0.3 * state->x;
    }
    
    if (state->x < MIN_DISTANCE_CM) state->x = MIN_DISTANCE_CM;
    if (state->x > MAX_DISTANCE_CM) state->x = MAX_DISTANCE_CM;
    
    state->p = (1 - state->k) * state->p;
}

// Modified echo pulse handler
void get_echo_pulse(uint gpio, uint32_t events) {
    if (gpio == ECHO_PIN) {
        if (events & GPIO_IRQ_EDGE_RISE) {
            start_time = get_absolute_time();
            measurement_valid = false;
        }
        else if (events & GPIO_IRQ_EDGE_FALL) {
            uint64_t current_time = to_us_since_boot(get_absolute_time());
            uint64_t start_time_us = to_us_since_boot(start_time);
            
            if (current_time > start_time_us) {
                pulse_width = current_time - start_time_us;
                measurement_valid = (pulse_width < MEASUREMENT_TIMEOUT_US);
            }
        }
    }
}

// Modified distance measurement function
float getCm() {
    if (distance_filter == NULL) {
        return 0.0;
    }

    measurement_valid = false;
    pulse_width = 0;
    
    gpio_put(TRIG_PIN, 0);
    sleep_us(2);
    gpio_put(TRIG_PIN, 1);
    sleep_us(10);
    gpio_put(TRIG_PIN, 0);
    
    absolute_time_t timeout_time = make_timeout_time_ms(15);
    while (!measurement_valid) {
        if (absolute_time_diff_us(get_absolute_time(), timeout_time) <= 0) {
            return distance_filter->x; // Return last estimate if measurement fails
        }
        tight_loop_contents();
    }
    
    double measured = (pulse_width * SPEED_OF_SOUND_CM_US) / 2.0;
    kalman_update(distance_filter, measured);
    
    printf("Raw: %.2f cm, Filtered: %.2f cm\n", measured, distance_filter->x);
    return (float)distance_filter->x;
}

// Function to check distance from the ultrasonic sensor and activate the buzzer if object is close
void measureDistanceAndBuzz() {
    uint64_t current_time = time_us_64();

    // Check if it's time to measure distance
    if (current_time - last_distance_check_time > CHECK_INTERVAL_MS * 1000) {
        last_distance_check_time = current_time;

        float distanceCm = getCm();
        if (distanceCm != 0.0 && distanceCm < DISTANCE_THRESHOLD_CM) {
            // Obstacle detected
            obstacle_detected = true;

            // Activate buzzer if not already on
            if (!buzzer_on) {
                gpio_put(BUZZER_PIN, 1);
                buzzer_on = true;
                buzzer_start_time = current_time;
            }
        } else {
            obstacle_detected = false;
        }
    }

    // Turn off buzzer after 200 ms
    if (buzzer_on && (current_time - buzzer_start_time >= 200000)) { // 200 ms
        gpio_put(BUZZER_PIN, 0);
        buzzer_on = false;
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

// Combined GPIO interrupt handler for both ultrasonic sensor and encoders
void gpio_interrupt_handler(uint gpio, uint32_t events) {
    // Handle ultrasonic sensor interrupts
    if (gpio == ECHO_PIN) {
        if (events & GPIO_IRQ_EDGE_RISE) {
            start_time = get_absolute_time();
            measurement_valid = false;
        }
        else if (events & GPIO_IRQ_EDGE_FALL) {
            uint64_t current_time = to_us_since_boot(get_absolute_time());
            uint64_t start_time_us = to_us_since_boot(start_time);
            
            if (current_time > start_time_us) {
                pulse_width = current_time - start_time_us;
                measurement_valid = (pulse_width < MEASUREMENT_TIMEOUT_US);
            }
        }
    }
    // Handle encoder interrupts
    else if (gpio == LEFT_ENCODER_PIN || gpio == RIGHT_ENCODER_PIN) {
        uint64_t current_time = time_us_64();

        if (gpio == LEFT_ENCODER_PIN) {
            left_pulse_count++;
            left_incremental_distance += DISTANCE_PER_PULSE_CM;
            left_total_distance += DISTANCE_PER_PULSE_CM;

            if (left_last_pulse_time != 0) {
                float time_diff_s = (current_time - left_last_pulse_time) / 1e6;
                left_speed_cm_s = DISTANCE_PER_PULSE_CM / time_diff_s;
            }
            left_last_pulse_time = current_time;
        } else {
            right_pulse_count++;
            right_incremental_distance += DISTANCE_PER_PULSE_CM;
            right_total_distance += DISTANCE_PER_PULSE_CM;

            if (right_last_pulse_time != 0) {
                float time_diff_s = (current_time - right_last_pulse_time) / 1e6;
                right_speed_cm_s = DISTANCE_PER_PULSE_CM / time_diff_s;
            }
            right_last_pulse_time = current_time;
        }
    }
}

// Modified setupEncoderPins function
void setupEncoderPins() {
    gpio_init(LEFT_ENCODER_PIN);
    gpio_set_dir(LEFT_ENCODER_PIN, GPIO_IN);
    gpio_set_irq_enabled_with_callback(LEFT_ENCODER_PIN, GPIO_IRQ_EDGE_RISE, true, &gpio_interrupt_handler);

    gpio_init(RIGHT_ENCODER_PIN);
    gpio_set_dir(RIGHT_ENCODER_PIN, GPIO_IN);
    gpio_set_irq_enabled(RIGHT_ENCODER_PIN, GPIO_IRQ_EDGE_RISE, true); // Note: no callback parameter here
}


// Modified setupUltrasonicPins function
void setupUltrasonicPins() {
    gpio_init(TRIG_PIN);
    gpio_init(ECHO_PIN);
    gpio_set_dir(TRIG_PIN, GPIO_OUT);
    gpio_set_dir(ECHO_PIN, GPIO_IN);
    gpio_pull_down(ECHO_PIN);
    gpio_set_irq_enabled(ECHO_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true); // Note: no callback parameter here
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
    uint64_t timeout = startWait + 5000; // 5 ms timeout

    // Wait for the echo to start
    while (gpio_get(ECHO_PIN) == 0) {
        if (time_us_64() > timeout) {
            // printf("Timeout: No echo detected\n");
            return 0;
        }
        tight_loop_contents();
    }

    uint64_t pulseStart = time_us_64();
    timeout = pulseStart + 5000; // 5 ms timeout

    // Wait for the echo to end
    while (gpio_get(ECHO_PIN) == 1) {
        if (time_us_64() > timeout) {
            // printf("Timeout: Echo duration exceeded\n");
            return 0;
        }
        tight_loop_contents();
    }
    uint64_t pulseEnd = time_us_64();

    // Return pulse duration
    return pulseEnd - pulseStart;
}


void right_encoder_callback(uint gpio, uint32_t events) {
    right_pulse_count++;
    right_incremental_distance += DISTANCE_PER_PULSE_CM;

    if (right_last_pulse_time != 0) {
        uint64_t current_time = time_us_64();
        float time_diff_s = (current_time - right_last_pulse_time) / 1e6;
        right_speed_cm_s = DISTANCE_PER_PULSE_CM / time_diff_s;

        // Debugging output
        //printf("Right Encoder Pulse Count: %d, Right Motor Speed: %.2f cm/s\n", right_pulse_count, right_speed_cm_s);

        right_last_pulse_time = current_time;
    }
}

// Initialization function for ultrasonic, encoder, and buzzer
void initializeBuddy5Components() {
    stdio_init_all();
    setupUltrasonicPins();
    setupEncoderPins();
    setupBuzzerPin();
    distance_filter = kalman_init(1.0, 0.5, 1.0, 20.0);
    last_distance_check_time = time_us_64();
}
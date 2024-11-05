#include "buddy5.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "hardware/gpio.h"
#include <math.h> // for M_PI

// Constants for measurement limits and filtering
#define MAX_DISTANCE_CM 400.0  // HC-SR04 max range is typically 400cm
#define MIN_DISTANCE_CM 2.0    // HC-SR04 min range is typically 2cm
#define SPEED_OF_SOUND_CM_US 0.0343  // Speed of sound in cm/microsecond
#define MEASUREMENT_TIMEOUT_US 25000  // Maximum time to wait for echo (25ms)

volatile absolute_time_t start_time;
volatile uint64_t pulse_width = 0;
volatile bool obstacleDetected = false;
volatile bool measurement_valid = false;


// Improved Kalman filter initialization with better default values
kalman_state *kalman_init(double q, double r, double p, double initial_value) {
    kalman_state *state = calloc(1, sizeof(kalman_state));
    if (state == NULL) {
        return NULL;
    }
    
    // Default values tuned for ultrasonic sensor characteristics
    state->q = q > 0 ? q : 1.0;  // Process noise - lower value for more stable output
    state->r = r > 0 ? r : 0.5;     // Measurement noise - higher value for noisier measurements
    state->p = p > 0 ? p : 1.0;     // Initial estimation error covariance
    state->x = initial_value;        // Initial state estimate
    return state;
}

/*
Too jumpy: Increase r to 0.75 or 1.0
Still too slow: Increase q to 1.5 or 2.0
Too noisy: Decrease q to 0.75
*/

// Improved interrupt handler with validity checking
void get_echo_pulse(uint gpio, uint32_t events) {
    if (gpio == ECHOPIN) {
        if (events & GPIO_IRQ_EDGE_RISE) {
            start_time = get_absolute_time();
            measurement_valid = false;
        }
        else if (events & GPIO_IRQ_EDGE_FALL) {
            uint64_t current_time = to_us_since_boot(get_absolute_time());
            uint64_t start_time_us = to_us_since_boot(start_time);
            
            // Check if the measurement is within valid range
            if (current_time > start_time_us) {
                pulse_width = current_time - start_time_us;
                measurement_valid = (pulse_width < MEASUREMENT_TIMEOUT_US);
            }
        }
    }
}

// Enhanced Kalman filter update with boundary checking
void kalman_update(kalman_state *state, double measurement) {
    if (state == NULL || measurement < MIN_DISTANCE_CM || measurement > MAX_DISTANCE_CM) {
        return;
    }

    // Calculate innovation (measurement - prediction)
    double innovation = measurement - state->x;
    
    // Adaptive process noise based on innovation
    if (fabs(innovation) > 10.0) {
        // Temporarily increase process noise for large changes
        state->q *= 2.0;
    } else {
        // Return to normal process noise
        state->q = 1.0;
    }

    // Prediction update with increased process noise
    state->p = state->p + state->q;

    // Measurement update
    state->k = state->p / (state->p + state->r);
    
    // More aggressive update for small innovations
    if (fabs(innovation) < 50.0) {
        state->x += state->k * innovation;
    } else {
        // For very large changes, trust the measurement more
        state->x = 0.7 * measurement + 0.3 * state->x;
    }
    
    // Ensure estimate stays within bounds
    if (state->x < MIN_DISTANCE_CM) state->x = MIN_DISTANCE_CM;
    if (state->x > MAX_DISTANCE_CM) state->x = MAX_DISTANCE_CM;
    
    // Update error covariance
    state->p = (1 - state->k) * state->p;
}


// Improved ultrasonic sensor setup
void setupUltrasonicPins() {
    gpio_init(TRIGPIN);
    gpio_init(ECHOPIN);
    gpio_set_dir(TRIGPIN, GPIO_OUT);
    gpio_set_dir(ECHOPIN, GPIO_IN);
    
    // Ensure trigger starts LOW
    gpio_put(TRIGPIN, 0);
    
    // Enable interrupt with proper pull-down
    gpio_pull_down(ECHOPIN);
    gpio_set_irq_enabled_with_callback(ECHOPIN, 
                                     GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
                                     true, 
                                     &get_echo_pulse);
}

uint64_t getPulse() {
    measurement_valid = false;
    pulse_width = 0;
    
    // Generate trigger pulse
    gpio_put(TRIGPIN, 1);
    sleep_us(10);
    gpio_put(TRIGPIN, 0);
    
    // Shorter timeout (15ms instead of 30ms)
    absolute_time_t timeout_time = make_timeout_time_ms(15);
    while (!measurement_valid) {
        if (absolute_time_diff_us(get_absolute_time(), timeout_time) <= 0) {
            return 0;
        }
        tight_loop_contents();
    }
    
    return pulse_width;
}

// Enhanced distance measurement with error handling
double getCm(kalman_state *state) {
    if (state == NULL) {
        return -1.0;
    }

    uint64_t pulseLength = getPulse();
    if (pulseLength == 0 || !measurement_valid) {
        // Keep previous estimate if measurement is invalid
        obstacleDetected = (state->x < 10.0);
        return state->x;
    }

    // Convert pulse width to distance using speed of sound
    double measured = (pulseLength * SPEED_OF_SOUND_CM_US) / 2.0;

    // Apply Kalman filter
    kalman_update(state, measured);
    
    // Update obstacle detection
    obstacleDetected = (measured < 10.0) || (state->x < 10.0);
    
    return state->x;
}


// // Example main function with error handling
// int main() {
//     stdio_init_all();
//     printf("Initializing ultrasonic sensor...\n");

//     setupUltrasonicPins();
    
//     // Initialize Kalman filter with tuned parameters
//     kalman_state *state = kalman_init(1.0, 0.5, 1.0, 20.0);
//     if (state == NULL) {
//         printf("Failed to initialize Kalman filter\n");
//         return 1;
//     }

//     sleep_ms(50);  // Brief settling time

//     while (true) {
//         double distance = getCm(state);
//         if (distance >= 0) {
//             printf("Distance: %.2f cm %s\n", 
//                    distance, 
//                    obstacleDetected ? "[OBSTACLE]" : "");
//         } else {
//             printf("Measurement error\n");
//         }
//         sleep_ms(50);  // 10Hz measurement rate
//     }

//     free(state);
//     return 0;
// }
#include "buddy2/buddy2.h" 
#include "hardware/clocks.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "buddy5/buddy5.h"         // Buddy5 motor control functions

// Define robot states
typedef enum {
    STATE_MOVING_FORWARD,
    STATE_TURNING_RIGHT,
    STATE_MOVING_FORWARD_DISTANCE,
    STATE_STOPPED
} RobotState;

RobotState current_state = STATE_MOVING_FORWARD;
uint64_t turning_start_time = 0;


// Variables for distance measurement

float target_distance_cm = 90.0f; // Distance to move forward in cm

// Function prototypes
void start_turning_right(void);
void reset_distance_counters(void);
bool turning_complete(void);

void reset_distance_counters() {
    left_pulse_count = 0;
    right_pulse_count = 0;
    left_incremental_distance = 0.0f;
    right_incremental_distance = 0.0f;
    left_total_distance = 0.0f;
    right_total_distance = 0.0f;
}

int main() {
    stdio_init_all();
    motor_control_init();
    
    // Initialize all Buddy5 components (includes Kalman filter)
    initializeBuddy5Components();
    
    // Reset PID controller variables
    integral_left = 0.0f;
    prev_error_left = 0.0f;

    // Set both motors to the same initial duty cycle
    set_pwm_duty_cycle(PWM_PIN, 0.94f);   // Left motor
    set_pwm_duty_cycle(PWM_PIN1, 0.99f);  // Right motor

    current_state = STATE_MOVING_FORWARD;

    sleep_ms(50); // Initial delay for system stabilization

    float current_distance = 0.0f;
    uint32_t last_print_time = 0;

    while (true) {
        // Measure distance and handle buzzer using Kalman-filtered measurements
        measureDistanceAndBuzz();
        current_distance = getCm();  // Get current filtered distance
        
        // Print debug information every 500ms
        uint32_t current_time = time_us_64() / 1000;
        if (current_time - last_print_time >= 500) {
            printf("Distance: %.2f cm, Left Speed: %.2f cm/s, Right Speed: %.2f cm/s\n", 
                   current_distance, left_speed_cm_s, right_speed_cm_s);
            last_print_time = current_time;
        }

        // Handle different robot states
        switch (current_state) {
            case STATE_MOVING_FORWARD:
                // Check if obstacle is detected at or before threshold
                if (current_distance <= 15) {
                    // Stop immediately when reaching threshold
                    printf("Obstacle detected at threshold (%.2f cm)! Stopping motors and starting turn.\n", current_distance);
                    set_pwm_duty_cycle(PWM_PIN, 0.0f);   // Left motor
                    set_pwm_duty_cycle(PWM_PIN1, 0.0f);  // Right motor
                    sleep_ms(500);

                    // Reset PID controller variables
                    integral_left = 0.0f;
                    prev_error_left = 0.0f;

                    // Start turning right
                    start_turning_right();
                    current_state = STATE_TURNING_RIGHT;
                } else {
                    // No obstacle at threshold, continue forward with speed adjustment
                    adjust_left_motor_speed();
                    
                    // Optional: Print distance for debugging
                    // printf("Current distance: %.2f cm\n", current_distance);
                }
                break;

            case STATE_TURNING_RIGHT:
                if (turning_complete()) {
                    reset_distance_counters();
                    // Transition to moving forward a specific distance
                    printf("Turn complete. Moving forward %.2f cm.\n", target_distance_cm);

                    // Set both motors forward
                    set_motor_direction(DIR_PIN1, DIR_PIN2, true);   // Left motor forward
                    set_motor_direction(DIR_PIN3, DIR_PIN4, true);   // Right motor forward

                    // Set initial duty cycles
                    set_pwm_duty_cycle(PWM_PIN, 0.94f);   // Left motor
                    set_pwm_duty_cycle(PWM_PIN1, 0.99f);  // Right motor


                    current_state = STATE_MOVING_FORWARD_DISTANCE;

                }
                break;

            case STATE_MOVING_FORWARD_DISTANCE: {
                // Print the current distance traveled for debugging
                if (current_time - last_print_time >= 500) {
                    printf("Current Distance Traveled: %.2f cm (Target: %.2f cm)\n", 
                           right_incremental_distance, target_distance_cm);
                }

                if (right_incremental_distance >= target_distance_cm) {
                    printf("Reached target distance of %.2f cm. Stopping.\n", target_distance_cm);
                    set_pwm_duty_cycle(PWM_PIN, 0.0f);    // Stop left motor
                    set_pwm_duty_cycle(PWM_PIN1, 0.0f);   // Stop right motor
                    current_state = STATE_STOPPED;
                }
                break;
            }

            case STATE_STOPPED:
                // Robot is stopped
                set_pwm_duty_cycle(PWM_PIN, 0.0f);   // Left motor
                set_pwm_duty_cycle(PWM_PIN1, 0.0f);  // Right motor
                sleep_ms(1000);
                break;
        }

        // Add a small delay to prevent CPU overutilization
        sleep_ms(1);
    }

    return 0;
}

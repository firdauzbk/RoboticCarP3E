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
int left_start_count = 0;
int right_start_count = 0;
float target_distance_cm = 90.0f; // Distance to move forward in cm

// Function prototypes
void start_turning_right(void);
bool turning_complete(void);
bool has_moved_distance(float distance_cm);

int main() {
    stdio_init_all();
    motor_control_init();
    initializeBuddy5Components();
    
    // Reset PID controller variables
    integral_left = 0.0f;
    prev_error_left = 0.0f;

    // Set both motors to the same initial duty cycle
    set_pwm_duty_cycle(PWM_PIN, 0.94f);   // Left motor
    set_pwm_duty_cycle(PWM_PIN1, 0.99f);  // Right motor

    current_state = STATE_MOVING_FORWARD;

    while (true) {
        // Measure distance and handle buzzer
        measureDistanceAndBuzz();

        // Handle different robot states
        switch (current_state) {
            case STATE_MOVING_FORWARD:
                if (!obstacle_detected) {
                    adjust_left_motor_speed();
                } else {
                    // Transition to turning state
                    printf("Obstacle detected! Stopping motors and starting turn.\n");
                    set_pwm_duty_cycle(PWM_PIN, 0.0f);   // Left motor
                    set_pwm_duty_cycle(PWM_PIN1, 0.0f);  // Right motor

                    // Reset PID controller variables
                    integral_left = 0.0f;
                    prev_error_left = 0.0f;

                    // Start turning right
                    start_turning_right();

                    current_state = STATE_TURNING_RIGHT;
                }
                break;

            case STATE_TURNING_RIGHT:
                if (turning_complete()) {
                    // Transition to moving forward a specific distance
                    printf("Turn complete. Moving forward 90 cm.\n");

                    // Set both motors forward
                    set_motor_direction(DIR_PIN1, DIR_PIN2, true);   // Left motor forward
                    set_motor_direction(DIR_PIN3, DIR_PIN4, true);   // Right motor forward

                    // Set initial duty cycles
                    set_pwm_duty_cycle(PWM_PIN, 0.94f);   // Left motor
                    set_pwm_duty_cycle(PWM_PIN1, 0.99f);  // Right motor

                    // Record starting encoder counts
                    left_start_count = left_pulse_count;
                    right_start_count = right_pulse_count;

                    current_state = STATE_MOVING_FORWARD_DISTANCE;
                }
                break;

            case STATE_MOVING_FORWARD_DISTANCE:
                if (!has_moved_distance(target_distance_cm)) {
                    adjust_left_motor_speed();
                } else {
                    // Stop the robot after moving the desired distance
                    printf("Moved 90 cm forward. Stopping.\n");
                    set_pwm_duty_cycle(PWM_PIN, 0.0f);   // Left motor
                    set_pwm_duty_cycle(PWM_PIN1, 0.0f);  // Right motor

                    current_state = STATE_STOPPED;
                }
                break;

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

bool has_moved_distance(float distance_cm) {
    // Calculate the distance traveled using encoder counts
    int left_ticks = left_pulse_count - left_start_count;
    int right_ticks = right_pulse_count - right_start_count;

    // Average the ticks from both wheels
    int average_ticks = (left_ticks + right_ticks) / 2;

    // Calculate distance traveled
    float distance_per_tick = DISTANCE_PER_PULSE_CM; // Already defined in buddy5.c/h
    float distance_traveled = average_ticks * distance_per_tick;

    // Debugging output (optional)
    // printf("Distance traveled: %.2f cm\n", distance_traveled);

    if (distance_traveled >= distance_cm) {
        return true;
    }
    return false;
}
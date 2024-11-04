// motor_pwm_pid.c
#include "buddy2.h"
#include "../buddy5/buddy5.h"
#include "hardware/clocks.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define RIGHT_MOTOR_CORRECTION_FACTOR 0.98f  // Adjust this value as needed

// PID constants for the left motor
float Kp = 2.0f;   // Proportional gain
float Ki = 0.05f;  // Integral gain
float Kd = 0.02f;  // Derivative gain

// PID variables for left motor adjustment
float integral_left = 0.0f;
float prev_error_left = 0.0f;

// Global variables to store duty cycles
float left_motor_duty_cycle = 0.0f;
float right_motor_duty_cycle = 0.0f;

// Global wrap value for PWM
uint16_t pwm_wrap_value = 65535; // Use a fixed wrap value

// Function to set PWM duty cycle and store duty cycle values
void set_pwm_duty_cycle(uint pwm_pin, float duty_cycle) {
    uint slice_num = pwm_gpio_to_slice_num(pwm_pin);
    uint chan = pwm_gpio_to_channel(pwm_pin);

    // Calculate the level based on the wrap value
    uint16_t level = duty_cycle * pwm_wrap_value;

    // Set the PWM channel level
    pwm_set_chan_level(slice_num, chan, level);

    // Store the duty cycle in the appropriate variable
    if (pwm_pin == PWM_PIN) {
        left_motor_duty_cycle = duty_cycle;
    } else if (pwm_pin == PWM_PIN1) {
        right_motor_duty_cycle = duty_cycle;
    }
    else {
        duty_cycle *= RIGHT_MOTOR_CORRECTION_FACTOR;
    }

    // Debugging output (optional)
    printf("Updated PWM on GPIO %d to %.2f%% duty cycle\n", pwm_pin, duty_cycle * 100);
}

// Function to estimate speed from duty cycle (if needed)
float estimate_speed_from_duty_cycle(float duty_cycle) {
    float max_speed = 100.0f; // Adjust based on your motor's specifications
    return duty_cycle * max_speed;
}

// Function to get right motor's duty cycle
float get_right_motor_duty_cycle() {
    return right_motor_duty_cycle;
}

// Function to adjust left motor speed using PID control
void adjust_left_motor_speed() {
    if (obstacle_detected) {
        // Do not adjust motor speed if obstacle is detected
        return;
    }

    // Use the speed calculated from both encoders
    float current_speed_left = left_speed_cm_s;
    float target_speed_left;

    // If right motor speed is less than threshold, estimate speed
    if (right_speed_cm_s < 1.0f) {
        // Estimate target speed based on right motor's duty cycle
        float right_duty_cycle = get_right_motor_duty_cycle();
        target_speed_left = estimate_speed_from_duty_cycle(right_duty_cycle);
        printf("Estimating target speed from duty cycle: %.2f cm/s\n", target_speed_left);
    } else {
        target_speed_left = right_speed_cm_s; // Target is the right motor's measured speed
    }

    // Debugging output
    printf("Current Left Motor Speed: %.2f cm/s\n", current_speed_left);
    printf("Target Left Motor Speed: %.2f cm/s\n", target_speed_left);

    // Compute PID adjustment
    float adjusted_duty_cycle = compute_pid(&target_speed_left, &current_speed_left, &integral_left, &prev_error_left);

    // Debugging output for adjusted duty cycle
    printf("Computed Left Motor Duty Cycle: %.2f%%\n", adjusted_duty_cycle * 100);

    // Apply adjusted duty cycle to left motor
    set_pwm_duty_cycle(PWM_PIN, adjusted_duty_cycle);
}

// Function to set up PWM for a given GPIO pin
void setup_pwm(uint gpio, float freq, float duty_cycle) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    uint chan = pwm_gpio_to_channel(gpio);

    // Calculate and set the PWM frequency and clock divider
    uint32_t f_sys = clock_get_hz(clk_sys);
    uint32_t divider16 = (f_sys * 16) / (freq * pwm_wrap_value);
    if (divider16 / 16 == 0) divider16 = 16; // Minimum divider value

    pwm_set_clkdiv_int_frac(slice_num, divider16 / 16, divider16 & 0xF);
    pwm_set_wrap(slice_num, pwm_wrap_value); // Use the global wrap value

    // Calculate the level based on the wrap value
    uint16_t level = duty_cycle * pwm_wrap_value;

    // Set the PWM channel level
    pwm_set_chan_level(slice_num, chan, level);

    pwm_set_enabled(slice_num, true);
    printf("PWM setup complete on GPIO %d with frequency %.2f Hz and duty cycle %.2f%%\n",
           gpio, freq, duty_cycle * 100);
}

// PID computation function
float compute_pid(float *target_speed, float *current_speed, float *integral, float *prev_error) {
    float error = *target_speed - *current_speed;
    *integral += error;

    // Integral clamping to avoid windup
    const float MAX_INTEGRAL = 1000.0f; // Adjust as needed
    if (*integral > MAX_INTEGRAL) *integral = MAX_INTEGRAL;
    if (*integral < -MAX_INTEGRAL) *integral = -MAX_INTEGRAL;

    float derivative = error - *prev_error;
    float duty_cycle = Kp * error + Ki * (*integral) + Kd * derivative;

    // Clamp duty cycle to [0, 0.99]
    if (duty_cycle > 0.99f) duty_cycle = 0.99f;
    else if (duty_cycle < 0.0f) duty_cycle = 0.0f;

    *prev_error = error;
    return duty_cycle;
}

// Motor direction control functions
void forward_motor_left() { set_motor_direction(DIR_PIN1, DIR_PIN2, true); }
void forward_motor_right() { set_motor_direction(DIR_PIN3, DIR_PIN4, true); }
void reverse_motor_left() { set_motor_direction(DIR_PIN1, DIR_PIN2, false); }
void reverse_motor_right() { set_motor_direction(DIR_PIN3, DIR_PIN4, false); }

void set_motor_direction(uint pin1, uint pin2, bool forward) {
    gpio_put(pin1, forward ? 1 : 0);
    gpio_put(pin2, forward ? 0 : 1);
    printf("Motor direction on pins %d and %d set to %s\n", pin1, pin2, forward ? "forward" : "reverse");
}

// Motor control initialization function
void motor_control_init(void) {
    // GPIO initialization
    gpio_init(PWM_PIN); gpio_init(PWM_PIN1);
    gpio_init(DIR_PIN1); gpio_init(DIR_PIN2);
    gpio_init(DIR_PIN3); gpio_init(DIR_PIN4);

    gpio_set_dir(PWM_PIN, GPIO_OUT); gpio_set_dir(PWM_PIN1, GPIO_OUT);
    gpio_set_dir(DIR_PIN1, GPIO_OUT); gpio_set_dir(DIR_PIN2, GPIO_OUT);
    gpio_set_dir(DIR_PIN3, GPIO_OUT); gpio_set_dir(DIR_PIN4, GPIO_OUT);

    // Set up PWM for both motors
    setup_pwm(PWM_PIN, 100.0f, 0.5f);  // Initialize left motor with 50% duty cycle
    setup_pwm(PWM_PIN1, 100.0f, 0.5f); // Initialize right motor with 50% duty cycle

    // Set both motors to move forward
    forward_motor_left();
    forward_motor_right();

    printf("Motor control initialized with both motors at 50%% duty cycle.\n");
}

// Turning functions
extern uint64_t turning_start_time; // This variable should be declared in your main file

void start_turning() {
    // Set motors to turn 90 degrees
    // Left motor forward, right motor reverse
    forward_motor_left();
    reverse_motor_right();

    // Set duty cycles for turning
    float turn_speed = 0.50f; // Adjust speed as needed
    set_pwm_duty_cycle(PWM_PIN, turn_speed);  // Left motor
    set_pwm_duty_cycle(PWM_PIN1, turn_speed); // Right motor

    // Record the start time
    turning_start_time = time_us_64();
}

bool turning_complete() {
    uint64_t current_time = time_us_64();
    uint64_t turn_duration = 1000000; // 1 second in microseconds; adjust as needed

    if (current_time - turning_start_time >= turn_duration) {
        // Stop motors
        set_pwm_duty_cycle(PWM_PIN, 0.0f);   // Left motor
        set_pwm_duty_cycle(PWM_PIN1, 0.0f);  // Right motor
        return true;
    }
    return false;
}

void start_turning_right() {
    // Start turning right using the same function
    start_turning();
}


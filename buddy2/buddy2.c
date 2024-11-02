#include "buddy2.h"
#include "../buddy5/buddy5.h"
#include "hardware/clocks.h"

#define DEFAULT_TARGET_SPEED 100.0f // Adjust based on your motor's characteristics

// PID constants for the left motor
float Kp = 0.7f;  // Increased proportional gain for faster response
float Ki = 0.02f; // Introduced integral gain for steady-state error correction
float Kd = 0.01f;


// Target speed for the left motor, matching the right motor's speed
float target_speed_left = 0.5f;  // Initially 50% of max speed

// PID variables for left motor adjustment
float integral_left = 0.0f;
float prev_error_left = 0.0f;

void set_pwm_duty_cycle(uint pwm_pin, float duty_cycle) {
    uint slice_num = pwm_gpio_to_slice_num(pwm_pin);
    uint chan = pwm_gpio_to_channel(pwm_pin);
    uint32_t wrap = 65535; // Use a fixed wrap value for simplicity

    // Calculate the level based on the wrap value
    uint16_t level = duty_cycle * wrap;

    // Set the PWM channel level
    pwm_set_chan_level(slice_num, chan, level);

    printf("Updated PWM on GPIO %d to %.2f%% speed\n", pwm_pin, duty_cycle * 100);
}


// Basic motor functions for right motor
void forward_motor_right() { set_motor_direction(DIR_PIN3, DIR_PIN4, true); }

void half_speed_right() { 
    set_pwm_duty_cycle(PWM_PIN1, 0.5f); // Set right motor to 50% duty cycle
    target_speed_left = 0.5f;  // Target the same speed for left motor
}

void gradual_ramp_up(uint pwm_pin, float start_duty_cycle, float max_duty_cycle, float increment, uint delay_ms) {
    float current_duty_cycle = start_duty_cycle;
    
    // Gradually increase the duty cycle to the maximum value
    while (current_duty_cycle < max_duty_cycle) {
        set_pwm_duty_cycle(pwm_pin, current_duty_cycle);
        printf("Ramping up PWM on GPIO %d to %.2f%% speed\n", pwm_pin, current_duty_cycle * 100);
        current_duty_cycle += increment;
        if (current_duty_cycle > max_duty_cycle) {
            current_duty_cycle = max_duty_cycle; // Ensure it doesn't exceed max value
        }
        sleep_ms(delay_ms);
    }

    // Set to the maximum value to finish the ramp-up
    set_pwm_duty_cycle(pwm_pin, max_duty_cycle);
    printf("Reached max speed on GPIO %d at %.2f%% speed\n", pwm_pin, max_duty_cycle * 100);
}

void full_speed_right() {
    float start_duty_cycle = 0.6f;   // Start at 20% speed
    float max_duty_cycle = 0.99f;    // Ramp up to 99% speed
    float increment = 0.05f;         // Increase by 5% in each step
    uint delay_ms = 1000;             // Delay between each increment (200 ms)

    // Gradually increase right motor speed
    gradual_ramp_up(PWM_PIN1, start_duty_cycle, max_duty_cycle, increment, delay_ms);
    
    // Update target speed for the left motor PID control
    target_speed_left = max_duty_cycle;
}


float compute_pid(float *target_speed, float *current_speed, float *integral, float *prev_error) {
    float error = *target_speed - *current_speed;
    *integral += error;

    // Integral clamping to avoid windup
    const float MAX_INTEGRAL = 43.7f;  // This is a tuning parameter
    if (*integral > MAX_INTEGRAL) *integral = MAX_INTEGRAL;
    if (*integral < -MAX_INTEGRAL) *integral = -MAX_INTEGRAL;

    float derivative = error - *prev_error;
    float duty_cycle = Kp * error + Ki * (*integral) + Kd * derivative;

    // Clamp duty cycle to [0, 1]
    if (duty_cycle > 1.0f) duty_cycle = 0.99f;
    else if (duty_cycle < 0.0f) duty_cycle = 0.0f;

    *prev_error = error;
    return duty_cycle;
}

void adjust_left_motor_speed() {
    // Use the speed calculated from both encoders
    float current_speed_left = left_speed_cm_s;
    float target_speed_left;

    // Handle initial condition where right_speed_cm_s is zero
    if (right_speed_cm_s < 1.0f) { // Threshold to consider speed as zero
        // Set a default target speed based on duty cycle
        target_speed_left = DEFAULT_TARGET_SPEED; // e.g., 100.0f cm/s
    } else {
        target_speed_left = right_speed_cm_s; // Target is the right motor's speed
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
    printf("Updated Left Motor PWM on GPIO %d to %.2f%% speed\n", PWM_PIN, adjusted_duty_cycle * 100);
}

void setup_pwm(uint gpio, float freq, float duty_cycle) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    uint chan = pwm_gpio_to_channel(gpio);

    // Calculate and set the PWM frequency and clock divider
    uint32_t f_sys = clock_get_hz(clk_sys);
    uint32_t divider16 = (f_sys * 16) / (freq * 65536);
    if (divider16 / 16 == 0) divider16 = 16; // Minimum divider value

    pwm_set_clkdiv_int_frac(slice_num, divider16 / 16, divider16 & 0xF);
    pwm_set_wrap(slice_num, 65535); // Use a fixed wrap value

    // Calculate the level based on the wrap value
    uint16_t level = duty_cycle * 65535;

    // Set the PWM channel level
    pwm_set_chan_level(slice_num, chan, level);

    pwm_set_enabled(slice_num, true);
    printf("PWM setup complete on GPIO %d with frequency %.2f Hz and duty cycle %.2f%%\n",
           gpio, freq, duty_cycle * 100);
}


// Motor direction control for left motor
void forward_motor_left() { set_motor_direction(DIR_PIN1, DIR_PIN2, true); }

void set_motor_direction(uint pin1, uint pin2, bool forward) {
    gpio_put(pin1, forward ? 1 : 0);
    gpio_put(pin2, forward ? 0 : 1);
    printf("Motor direction set to %s\n", forward ? "forward" : "reverse");
}



void motor_control_init(void) {
    // GPIO initialization
    gpio_init(PWM_PIN); gpio_init(PWM_PIN1);
    gpio_init(DIR_PIN1); gpio_init(DIR_PIN2);
    gpio_init(DIR_PIN3); gpio_init(DIR_PIN4);

    gpio_set_dir(PWM_PIN, GPIO_OUT); gpio_set_dir(PWM_PIN1, GPIO_OUT);
    gpio_set_dir(DIR_PIN1, GPIO_OUT); gpio_set_dir(DIR_PIN2, GPIO_OUT);
    gpio_set_dir(DIR_PIN3, GPIO_OUT); gpio_set_dir(DIR_PIN4, GPIO_OUT);

    // Set up PWM for both motors
    setup_pwm(PWM_PIN, 100.0f, 0.5f);  // Initialize left motor with 0% duty cycle
    setup_pwm(PWM_PIN1, 100.0f, 0.5f); // Initialize right motor to 50% duty cycle

    // Set both motors forward direction
    forward_motor_left();
    forward_motor_right();

    printf("Motor control initialized with right motor at 50%%, left motor adjusting.\n");
}

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "buddy2.h"

// Define GPIO pins
#define PWM_PIN 2     // GP2 for PWM (Motor 1)
#define DIR_PIN1 0    // GP0 for direction
#define DIR_PIN2 1    // GP1 for direction

#define PWM_PIN1 5    // GP5 for PWM (Motor 2)
#define DIR_PIN3 3    // GP3 for direction
#define DIR_PIN4 4    // GP4 for direction
#define START_STOP_PIN 20 // GP20 for Start/Stop button
#define INTERUPT_PIN 21 // GP21 for interupt

float current_speed = 0.0f;
float current_speed1 = 0.0f;
bool motor_running = true;

// Function to set up the PWM
void setup_pwm(uint gpio, float freq, float duty_cycle) {
    // Set the GPIO function to PWM
    gpio_set_function(gpio, GPIO_FUNC_PWM);

    // Find out which PWM slice is connected to the specified GPIO
    uint slice_num = pwm_gpio_to_slice_num(gpio);

    // Calculate the PWM frequency and set the PWM wrap value
    float clock_freq = 125000000.0f;  // Default Pico clock frequency in Hz
    uint32_t divider = clock_freq / (freq * 65536);  // Compute divider for given frequency
    pwm_set_clkdiv(slice_num, divider);

    // Set the PWM wrap value (maximum count value)
    pwm_set_wrap(slice_num, 65535);  // 16-bit counter (0 - 65535)

    // Set the duty cycle
    pwm_set_gpio_level(gpio, (uint16_t)(duty_cycle * 65536));

    // Enable the PWM
    pwm_set_enabled(slice_num, true);
}

// For Left Motor
void forward_motor_left() {
    // Set the direction pins for forward motion
    gpio_put(DIR_PIN1, 1);
    gpio_put(DIR_PIN2, 0);
    printf("Left Forward\n");
}

void reverse_motor_left() {
    // Set the direction pins for reverse motion
    gpio_put(DIR_PIN1, 0);
    gpio_put(DIR_PIN2, 1);
    printf("Left Reverse\n");
}

void update_speed_left(float speed){
    pwm_set_gpio_level(PWM_PIN, (uint16_t)(speed * 65536));
}

void full_speed_left(){
    printf("Left Speed: 100%% Full Speed\n");
    current_speed = 0.99f;
    update_speed_left(current_speed);
}

void half_speed_left(){
    printf("Left Speed: 50%% Half Speed\n");
    current_speed = 0.5f;
    update_speed_left(current_speed);
}

void stop_motor_left(){
    printf("Left Speed: 0%% Motor Stop\n");
    current_speed = 0.0f;
    update_speed_left(current_speed);
}

// For Right Motor
void forward_motor_right() {
    // Set the direction pins for forward motion
    gpio_put(DIR_PIN3, 1);
    gpio_put(DIR_PIN4, 0);
    printf("Right Forward\n");
}

void reverse_motor_right() {
    // Set the direction pins for reverse motion
    gpio_put(DIR_PIN3, 0);
    gpio_put(DIR_PIN4, 1);
    printf("Right Reverse\n");
}

void update_speed_right(float speed){
    pwm_set_gpio_level(PWM_PIN1, (uint16_t)(speed * 65536));
}

void full_speed_right(){
    printf("Right speed: 100%% Full Speed\n");
    current_speed1 = 0.99f;
    update_speed_right(current_speed1);
}

void half_speed_right(){
    printf("Right Speed: 50%% Half Speed\n");
    current_speed1 = 0.5f;
    update_speed_right(current_speed1);
}

void stop_motor_right(){
    printf("Right Speed: 0%% Motor Stop\n");
    current_speed1 = 0.0f;
    update_speed_right(current_speed1);
}

// Setting direction and speed of left motor

void left_full_motor_forward(){
    forward_motor_left();
    full_speed_left();
}

void left_full_motor_reverse(){
    reverse_motor_left();
    full_speed_left();
}

void left_half_motor_forward(){
    forward_motor_left();
    half_speed_left();
}

void left_half_motor_reverse(){
    reverse_motor_left();
    half_speed_left();
}

void left_stop_motor(){
    stop_motor_left();
}

// Setting direction and speed of right motor

void right_full_motor_forward(){
    forward_motor_right();
    full_speed_right();
}

void right_full_motor_reverse(){
    reverse_motor_right();
    full_speed_right();
}

void right_half_motor_forward(){
    forward_motor_right();
    half_speed_right();
}

void right_half_motor_reverse(){
    reverse_motor_right();
    half_speed_right();
}

void right_stop_motor(){
    stop_motor_right();
}

// Setting direction for both motors

void both_full_motor_forward(){
    left_full_motor_forward();
    right_full_motor_forward();
}

void both_full_motor_reverse(){
    left_full_motor_reverse();
    right_full_motor_reverse();
}

void both_half_motor_forward(){
    left_half_motor_forward();
    right_half_motor_forward();
}

void both_half_motor_reverse(){
    left_half_motor_reverse();
    right_half_motor_reverse();
}

void both_stop_motor(){
    left_stop_motor();
    right_stop_motor();
}

// Setting direction and movement for left and stop for right
void left_full_forward_right_stop(){
    left_full_motor_forward();
    right_stop_motor();
}

void left_full_reverse_right_stop(){
    left_full_motor_reverse();
    right_stop_motor();
}

void left_half_forward_right_stop(){
    left_half_motor_forward();
    right_stop_motor();
}

void left_half_reverse_right_stop(){
    left_half_motor_reverse();
    right_stop_motor();
}

// Setting direction and movement for right and stop for left

void right_full_forward_left_stop(){
    right_full_motor_forward();
    left_stop_motor();
}

void right_full_reverse_left_stop(){
    right_full_motor_reverse();
    left_stop_motor();
}

void right_half_forward_left_stop(){
    right_half_motor_forward();
    left_stop_motor();
}

void right_half_reverse_left_stop(){
    right_half_motor_reverse();
    left_stop_motor();
}

// One full speed one half speed

void left_full_right_half_foward(){
    left_full_motor_forward();
    right_half_motor_forward();
}

void left_half_right_full_forward(){
    left_half_motor_forward();
    right_full_motor_forward();
}

void left_full_right_half_reverse(){
    left_full_motor_reverse();
    right_half_motor_reverse();
}

void left_half_right_full_reverse(){
    left_half_motor_reverse();
    right_full_motor_reverse();
}

// Moving on the spot

void left_forward_right_reverse_full(){
    left_full_motor_forward();
    right_full_motor_reverse();
}

void left_reverse_right_forward_full(){
    left_full_motor_reverse();
    right_full_motor_forward();
}

void left_forward_right_reverse_half(){
    left_half_motor_forward();
    right_half_motor_reverse();
}

void left_reverse_right_forward_half(){
    left_half_motor_reverse();
    right_half_motor_forward();
}

// Interrpt handler
void interrupt_handler(uint gpio, uint32_t events){
    printf("Interupt\n");
    if (gpio == INTERUPT_PIN && events == GPIO_IRQ_EDGE_RISE){
        printf("Stop Both Motor\n");
        stop_motor_left();
        stop_motor_right();
        pwm_set_gpio_level(PWM_PIN, 0);
        pwm_set_gpio_level(PWM_PIN1, 0);
    }
}
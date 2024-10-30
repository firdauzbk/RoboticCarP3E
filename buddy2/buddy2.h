#ifndef BUDDY2_H
#define BUDDY2_H

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"

// Define GPIO pins
#define PWM_PIN 2     // GP2 for PWM (Motor 1)
#define DIR_PIN1 0    // GP0 for direction
#define DIR_PIN2 1    // GP1 for direction

#define PWM_PIN1 5    // GP5 for PWM (Motor 2)
#define DIR_PIN3 3    // GP3 for direction
#define DIR_PIN4 4    // GP4 for direction
#define START_STOP_PIN 20 // GP20 for Start/Stop button
#define INTERUPT_PIN 21 // GP21 for interrupt

// Variables
extern float current_speed;
extern float current_speed1;
extern bool motor_running;

// Function prototypes
void setup_pwm(uint gpio, float freq, float duty_cycle);

// Left motor control functions
void forward_motor_left();
void reverse_motor_left();
void update_speed_left(float speed);
void full_speed_left();
void half_speed_left();
void stop_motor_left();

// Right motor control functions
void forward_motor_right();
void reverse_motor_right();
void update_speed_right(float speed);
void full_speed_right();
void half_speed_right();
void stop_motor_right();

// Combined motor control functions
void left_full_motor_forward();
void left_full_motor_reverse();
void left_half_motor_forward();
void left_half_motor_reverse();
void left_stop_motor();

void right_full_motor_forward();
void right_full_motor_reverse();
void right_half_motor_forward();
void right_half_motor_reverse();
void right_stop_motor();

void both_full_motor_forward();
void both_full_motor_reverse();
void both_half_motor_forward();
void both_half_motor_reverse();
void both_stop_motor();

void left_full_forward_right_stop();
void left_full_reverse_right_stop();
void left_half_forward_right_stop();
void left_half_reverse_right_stop();

void right_full_forward_left_stop();
void right_full_reverse_left_stop();
void right_half_forward_left_stop();
void right_half_reverse_left_stop();

void left_full_right_half_forward();
void left_half_right_full_forward();
void left_full_right_half_reverse();
void left_half_right_full_reverse();

void left_forward_right_reverse_full();
void left_reverse_right_forward_full();
void left_forward_right_reverse_half();
void left_reverse_right_forward_half();

// Interrupt handler
void interrupt_handler(uint gpio, uint32_t events);

#endif // BUDDY2_H

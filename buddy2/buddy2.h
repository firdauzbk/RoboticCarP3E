#ifndef BUDDY2_H
#define BUDDY2_H

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "../buddy5/buddy5.h"

// Define GPIO pins for motors
#define PWM_PIN 2          // GP2 for PWM (Motor 1 - Left Motor)
#define DIR_PIN1 17        // GP17 for direction (Motor 1 - Left Motor)
#define DIR_PIN2 16        // GP16 for direction (Motor 1 - Left Motor)
#define PWM_PIN1 3         // GP3 for PWM (Motor 2 - Right Motor)
#define DIR_PIN3 14        // GP14 for direction (Motor 2 - Right Motor)
#define DIR_PIN4 15        // GP15 for direction (Motor 2 - Right Motor)

// PID control constants
extern float Kp;
extern float Ki;
extern float Kd;

// PID function declaration
float compute_pid(float *target_speed, float *current_speed, float *integral, float *prev_error);

// Motor control functions
void motor_control_init(void);
void forward_motor_right(void); // Set right motor to a constant speed
void half_speed_right(void); // Set right motor to 50% speed
void adjust_left_motor_speed(void); // Adjust left motor to match right motor
void set_motor_direction(uint pin1, uint pin2, bool forward);
void set_pwm_duty_cycle(uint pwm_pin, float duty_cycle);
void setup_pwm(uint gpio, float freq, float duty_cycle); 
void set_left_motor_target_speed(float speed); // Set target speed for left motor
void gradual_ramp_up(uint pwm_pin, float start_duty_cycle, float max_duty_cycle, float increment, uint delay_ms);

// Motor control functions (Left Motor)
void forward_motor_left();
void reverse_motor_left();
void stop_motor_left();
void full_speed_left();
void half_speed_left();

// Motor control functions (Right Motor)
void forward_motor_right();
void reverse_motor_right();
void stop_motor_right();
void full_speed_right();
void half_speed_right();

// Control functions for both motors
void both_stop_motor();
void both_full_motor_forward();
void both_half_motor_forward();
void both_full_motor_reverse();
void both_half_motor_reverse();

void motor_control_init(void);


#endif // BUDDY2_H

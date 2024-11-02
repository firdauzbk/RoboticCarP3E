#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "buddy1/buddy1.h"
#include "buddy2/buddy2.h"
//#include "buddy3/buddy3.h"
#include "buddy4/buddy4.h"
#include "buddy5/buddy5.h"


int main() {
    // Initialize standard I/O
    stdio_init_all();

    // Buddy1: Initialize motor control (if needed)
    motor_control_init();

    // Buddy2: Set up motor direction and PWM pins
    set_pwm_duty_cycle(PWM_PIN, 0.94f);   // Set right motor to 50% speed
    set_pwm_duty_cycle(PWM_PIN1, 0.99f);  // Set left motor to 50% speed

    // Buddy3: Initialize ADC for IR sensors
    //setup_adc();

    // Buddy5: Initialize ultrasonic sensor, encoders, and buzzer
    initializeBuddy5Components();

    while (1) {
        // Buddy2: Adjust left motor speed to match the right motor
        adjust_left_motor_speed();

        // Buddy3: Read IR sensors to follow a line based on surface contrast
        //read_ir_sensors();

        // Buddy5: Check distance and activate buzzer if necessary
        measureDistanceAndBuzz();

        // Buddy5: Output speed and total distance for each wheel
        printf("Left Wheel - Speed: %.2f cm/s, Total Distance: %.2f cm\n", 
               left_speed_cm_s, left_total_distance);
        printf("Right Wheel - Speed: %.2f cm/s, Total Distance: %.2f cm\n", 
               right_speed_cm_s, right_total_distance);

        // Adjust delay as needed for testing
        sleep_ms(500);  
    }

    return 0;
}
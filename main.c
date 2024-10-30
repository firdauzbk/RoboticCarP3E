#include <stdio.h>
#include "pico/stdlib.h"
#include "buddy1/buddy1.h"
#include "buddy2/buddy2.h"
#include "buddy3/buddy3.h"
#include "buddy4/buddy4.h"
#include "buddy5/buddy5.h"

int main() {
    // Initialize standard I/O
    stdio_init_all();
    setup_adc(); 
    gpio_init(DIR_PIN1);
    gpio_init(DIR_PIN2);
    gpio_set_dir(DIR_PIN1, GPIO_OUT);
    gpio_set_dir(DIR_PIN2, GPIO_OUT);

    // Initialize GPIO pins for right direction control
    gpio_init(DIR_PIN3);
    gpio_init(DIR_PIN4);
    gpio_set_dir(DIR_PIN3, GPIO_OUT);
    gpio_set_dir(DIR_PIN4, GPIO_OUT);

    // Initialize GPIO pin for interupt
    gpio_init(INTERUPT_PIN);
    gpio_set_dir(INTERUPT_PIN, GPIO_IN);
    gpio_pull_up(INTERUPT_PIN);

    // Set up interupt handler
    gpio_set_irq_enabled_with_callback(INTERUPT_PIN, GPIO_IRQ_EDGE_RISE, true, &interrupt_handler);

    // Set up PWM on GPIO0
    setup_pwm(PWM_PIN, 100.0f, current_speed);  // 100 Hz frequency, 50% duty cycle

    // Set up PWM on GPIO5
    setup_pwm(PWM_PIN1, 100.0f, current_speed1);  // 100 Hz frequency, 50% duty cycle


    while (1) {
        read_ir_sensors(); // Read IR sensor values
        both_full_motor_forward();
        buddy1_function();
        buddy4_function();
        buddy5_function();
        sleep_ms(1000);
    }
    return 0; 
}
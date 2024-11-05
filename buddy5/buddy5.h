#ifndef BUDDY5_H
#define BUDDY5_H

#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "../buddy2/buddy2.h"
/* ---------------- ULTRASONIC ----------------*/
// Define GPIO pins for the ultrasonic sensor
#define TRIGPIN 4
#define ECHOPIN 5

// Kalman filter state structure
typedef struct kalman_state_ {
    double q; // Process noise covariance
    double r; // Measurement noise covariance
    double x; // Estimated value
    double p; // Estimation error covariance
    double k; // Kalman gain
} kalman_state;

/* ---------------- IR WHEEL ENCODER ----------------*/

// Encoder pin definitions for left and right wheels
#define LEFT_ENCODER_PIN 8                 // GPIO pin for left wheel encoder (GP8)
#define RIGHT_ENCODER_PIN 0                // GPIO pin for right wheel encoder (GP0)

// Wheel and encoder specifications
extern const float WHEEL_DIAMETER_CM;         // Diameter of the wheel in cm
extern const float WHEEL_CIRCUMFERENCE_CM;    // Circumference of the wheel in cm
extern const unsigned int SLOTS_PER_REVOLUTION; // Number of slots in encoder wheel
extern const float DISTANCE_PER_PULSE_CM;     // Distance traveled per pulse in cm

// Global variables for distance and speed tracking (left and right)
extern volatile int left_pulse_count;
extern volatile float left_incremental_distance;
extern volatile float left_total_distance;
extern volatile float left_speed_cm_s;
extern volatile uint64_t left_last_pulse_time;

extern volatile int right_pulse_count;
extern volatile float right_incremental_distance;
extern volatile float right_total_distance;
extern volatile float right_speed_cm_s;
extern volatile uint64_t right_last_pulse_time;

// External flag to indicate if an obstacle is detected
extern volatile bool obstacleDetected;

// Function declarations for Buddy5
void initializeBuddy5Components(void);    // Initializes ultrasonic, encoder, and buzzer components
void updateLastCheckTime(void);           // Manually updates the last check time

// Internal helper functions (not required to be called from main)
kalman_state *kalman_init(double q, double r, double p, double initial_value);
void get_echo_pulse(uint gpio, uint32_t events);
void kalman_update(kalman_state *state, double measurement);
void setupUltrasonicPins();
uint64_t getPulse();
double getCm(kalman_state *state);

void setupEncoderPins(void);
void right_encoder_callback(uint gpio, uint32_t events);

#endif // BUDDY5_H

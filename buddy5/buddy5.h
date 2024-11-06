#ifndef BUDDY5_H
#define BUDDY5_H

#include <stdint.h>
#include <stdbool.h>
#include "../buddy2/buddy2.h"

// Kalman filter state structure
typedef struct kalman_state_ {
    double q; // Process noise covariance
    double r; // Measurement noise covariance
    double x; // Estimated value
    double p; // Estimation error covariance
    double k; // Kalman gain
} kalman_state;

// Constants for measurement limits and filtering
#define MAX_DISTANCE_CM 400.0
#define MIN_DISTANCE_CM 2.0
#define SPEED_OF_SOUND_CM_US 0.0343
#define MEASUREMENT_TIMEOUT_US 25000

// Ultrasonic sensor pin definitions
extern const unsigned int TRIG_PIN;        // GPIO pin for ultrasonic trigger (GP4)
extern const unsigned int ECHO_PIN;        // GPIO pin for ultrasonic echo (GP5)

// Encoder pin definitions for left and right wheels
#define LEFT_ENCODER_PIN 8                 // GPIO pin for left wheel encoder (GP8)
#define RIGHT_ENCODER_PIN 0                // GPIO pin for right wheel encoder (GP0)

// Buzzer pin definition
extern const unsigned int BUZZER_PIN;      // GPIO pin for buzzer

// Distance threshold and check interval
extern const float DISTANCE_THRESHOLD_CM;  // Distance threshold in cm for buzzer activation
extern const unsigned int CHECK_INTERVAL_MS;  // Interval in ms between distance checks

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

// Kalman filter and measurement variables
extern volatile absolute_time_t start_time;
extern volatile uint64_t pulse_width;
extern volatile bool measurement_valid;

// Obstacle detection flag
extern volatile bool obstacle_detected;

// Function declarations for Kalman filter
kalman_state *kalman_init(double q, double r, double p, double initial_value);
void kalman_update(kalman_state *state, double measurement);
void get_echo_pulse(uint gpio, uint32_t events);

// Main function declarations
void initializeBuddy5Components(void);    // Initializes ultrasonic, encoder, and buzzer components
void measureDistanceAndBuzz(void);        // Measures distance and activates buzzer if too close
void updateLastCheckTime(void);           // Manually updates the last check time

// Internal helper functions
void setupUltrasonicPins(void);
void setupBuzzerPin(void);
void setupEncoderPins(void);
float getCm(void);
void right_encoder_callback(uint gpio, uint32_t events);

#endif // BUDDY5_H
#ifndef BUDDY5_H
#define BUDDY5_H

#include <stdint.h>
#include <stdbool.h>
#include "../buddy2/buddy2.h"

// #define DISTANCE_THRESHOLD_CM 10.0
// #define CHECK_INTERVAL_MS 200
// buddy5.h

// Existing includes and declarations...


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


extern volatile bool obstacle_detected;
// Function declarations for Buddy5
void initializeBuddy5Components(void);    // Initializes ultrasonic, encoder, and buzzer components
void measureDistanceAndBuzz(void);        // Measures distance and activates buzzer if too close
void updateLastCheckTime(void);           // Manually updates the last check time

// Internal helper functions (not required to be called from main)
void setupUltrasonicPins(void);
void setupBuzzerPin(void);
void setupEncoderPins(void);
float getCm(void);
uint64_t getPulse(void);
void right_encoder_callback(uint gpio, uint32_t events);

#endif // BUDDY5_H

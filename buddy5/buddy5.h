#ifndef BUDDY5_H
#define BUDDY5_H

#include <stdint.h>

#define DISTANCE_THRESHOLD_CM 10.0
#define CHECK_INTERVAL_MS 200

// Ultrasonic sensor pin definitions
extern const unsigned int TRIG_PIN;        // GPIO pin for ultrasonic trigger (GP4)
extern const unsigned int ECHO_PIN;        // GPIO pin for ultrasonic echo (GP5)

// Encoder pin definitions
extern const unsigned int ENCODER_PIN_A;   // GPIO pin A for encoder (GP2)
extern const unsigned int ENCODER_PIN_B;   // GPIO pin B for encoder (GP3)

// Buzzer pin definition
extern const unsigned int BUZZER_PIN;      // GPIO pin for buzzer

// // Distance threshold and check interval
// extern const float DISTANCE_THRESHOLD_CM;  // Distance threshold in cm for buzzer activation
// extern const unsigned int CHECK_INTERVAL_MS;  // Interval in ms between distance checks

// Wheel and encoder specifications
extern const float WHEEL_DIAMETER_CM;         // Diameter of the wheel in cm
extern const float WHEEL_CIRCUMFERENCE_CM;    // Circumference of the wheel in cm
extern const unsigned int SLOTS_PER_REVOLUTION; // Number of slots in encoder wheel
extern const float DISTANCE_PER_PULSE_CM;     // Distance traveled per pulse in cm

// Global variables for distance tracking
extern volatile int pulse_count;               // Pulse count for encoder
extern volatile float incremental_distance;    // Incremental distance for current revolution
extern volatile float total_distance;          // Cumulative total distance
extern volatile float last_distance_traveled;  // Distance for last revolution

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

#endif // BUDDY5_H

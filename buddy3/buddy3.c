#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include <string.h> 
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "buddy3.h"
#include "buddy2.h"

#define LEFT_IR_SENSOR_ANALOG_PIN 26   // ADC GPIO pin for the left sensor
#define RIGHT_IR_SENSOR_ANALOG_PIN 27   // ADC GPIO pin for the right sensor

// Fixed threshold for surface detection
const int BLACK_WHITE_THRESHOLD = 200;

// Variables to track sensor states
bool last_state_black[2] = {false, false}; // Last states for left and right
bool detect_barcode = false;

// Function prototypes
void setup_adc();
uint16_t read_adc(int sensor_index);
void read_ir_sensors();
void line_following(uint16_t analog_values[]);
void print_detected_state(int sensor_index, bool current_state_black, uint16_t analog_value, float voltage);
int find_binary_index(const char *binary_code, char *code_array[]);
char map_binary_to_char(const char *binary_code, bool reverse);
void barcode_detector(uint16_t analog_value);
void convert_stay_counts(int stay_counts[], int barcount, bool *direction);

// Function to set up ADC
void setup_adc() {
    adc_init();
    adc_gpio_init(LEFT_IR_SENSOR_ANALOG_PIN);
    adc_gpio_init(RIGHT_IR_SENSOR_ANALOG_PIN);
}

// Function to select ADC input based on sensor index
uint16_t read_adc(int sensor_index) {
    adc_select_input(sensor_index); // 0 for left, 1 for right
    return adc_read(); // Read the selected ADC input
}

// Function to read IR sensors and print current state and values
void read_ir_sensors() {
    uint16_t analog_values[2];
    float voltages[2];

    // Read analog values from the sensors
    for (int i = 0; i < 2; i++) {
        analog_values[i] = read_adc(i); // Read from the corresponding sensor
        voltages[i] = analog_values[i] * (3.3f / 4095.0f);  // Convert analog value to voltage
        bool current_state_black = (analog_values[i] > BLACK_WHITE_THRESHOLD);
        // print_detected_state(i, current_state_black, analog_values[i], voltages[i]);
        last_state_black[i] = current_state_black; // Update last state

        // Start barcode detection
        if (i == 0 && current_state_black) {
             detect_barcode = true;
         }

        if(i == 0 && detect_barcode){
             barcode_detector(analog_values[i]);
         }

        // Start line following
        // line_following(analog_values);
    }
}

void print_detected_state(int sensor_index, bool current_state_black, uint16_t analog_value, float voltage) {
    const char* sensor_name = (sensor_index == 0) ? "Barcode Detector Sensor" : "Line Following Sensor";

    // Testing
    if (sensor_index != 0) {
        return;
    }

    if (current_state_black) {
        printf("%s - Detected: Black Surface\n", sensor_name);
    } else {
        printf("%s - Detected: White Surface\n", sensor_name);
    }

    // Print the raw analog value and voltage for debugging
    printf("%s - Analog Value: %u, Voltage: %.2fV\n", sensor_name, analog_value, voltage);
}

// Function to control line following based on right sensor
void line_following(uint16_t analog_value[]) {
    bool on_black = (analog_value[1] > BLACK_WHITE_THRESHOLD);

    if (on_black) {
        printf("On black surface. Moving forward...\n");
        both_full_motor_forward();
    } else {
        printf("On white surface. Stopping...\n");
        both_stop_motor();
    }
}

#define UNMAPPED_CHAR '?'

// Initialise array used to store each barcode character
char array_char[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
                                'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', '_', '.', '$', '/', '+', '%', ' ', '*'}; 

// Initialise array used to store binary representation of each character
char *array_code[] = {"000110100", "100100001", "001100001", "101100000", "000110001", "100110000", "001110000",
                                "000100101", "100100100", "001100100", "100001001", "001001001", "101001000", "000011001",
                                "100011000", "001011000", "000001101", "100001100", "001001100", "000011100", "100000011",
                                "001000011", "101000010", "000010011", "100010010", "001010010", "000000111", "100000110",
                                "001000110", "000010110", "110000001", "011000001", "111000000", "010010001", "110010000",
                                "011010000", "010000101", "110000100", "010101000", "010100010", "010001010", "000101010",
                                "011000100", "010010100"}; 

// Initialise array used to store the reversed binary representation of each character
char *array_reverse_code[] = {"001011000", "100001001", "100001100", "000001101", "100011000", "000011001",
                                        "000011100", "101001000", "001001001", "001001100", "100100001", "100100100",
                                        "000100101", "100110000", "000110001", "000110100", "101100000", "001100001",
                                        "001100100", "001110000", "110000001", "110000100", "010000101", "110010000",
                                        "010010001", "010010100", "111000000", "011000001", "011000100", "011010000",
                                        "100000011", "100000110", "000000111", "100010010", "000010011", "000010110",
                                        "101000010", "001000011", "000101010", "010001010", "010100010", "010101000",
                                        "001000110", "001010010"};

// Function to find the index of a binary code in the specified code array
int find_binary_index(const char *binary_code, char *code_array[]) {
    for (int i = 0; i < sizeof(array_code) / sizeof(array_code[0]); i++) {
        if (strcmp(code_array[i], binary_code) == 0) {
            return i; // Return index if binary code is found
        }
    }
    return -1; // Return -1 if binary code is not found
}

// Function to map binary code to its corresponding character with an option for reverse mapping
char map_binary_to_char(const char *binary_code, bool reverse) {
    // Choose the appropriate array based on the reverse flag
    char **code_array = reverse ? array_reverse_code : array_code;
    
    int index = find_binary_index(binary_code, code_array); // Find the index in the selected array

    if (index != -1) {
        return array_char[index]; // Return the corresponding character
    }
    
    // Return "?" if the binary code is not found
    return UNMAPPED_CHAR; 
}

#define MAX_TRANSITIONS 27 // Set the max transitions to 36
#define CHUNK_SIZE 9       // Every 9 bars will map to a character
#define TOP_K 3           // Specify the value of k for conversion
#define CHAR_COUNT (MAX_TRANSITIONS / CHUNK_SIZE) // Number of characters expected

int barcount = 0;
bool barcode_black_detected = false; // To track the last state
int stay_count = 0;                  // Variable to track how long the sensor stays in the same state
int stay_counts[MAX_TRANSITIONS];    // Array to store stay counts for each transition
char converted_chars[CHAR_COUNT];    // Array to store each mapped character
int char_index = 0;                  // Index for storing characters

// Global or static variable to keep track of the direction
bool direction_determined = false;

// Function to convert stay counts for each 9-bar chunk and map to character
void convert_stay_counts(int stay_counts[], int barcount, bool *direction) {
    // Ensure barcount is a multiple of CHUNK_SIZE
    if (barcount % CHUNK_SIZE != 0) {
        printf("Error: barcount is not a multiple of CHUNK_SIZE.\n");
        return;
    }

    // Process each chunk of CHUNK_SIZE
    for (int chunk_start = 0; chunk_start < barcount; chunk_start += CHUNK_SIZE) {
        int converted_counts[CHUNK_SIZE] = {0};
        int top_counts[CHUNK_SIZE];

        // Initialize top_counts array for sorting
        for (int i = 0; i < CHUNK_SIZE; i++) {
            top_counts[i] = stay_counts[chunk_start + i];
        }

        // Sort to find the top K values in the current chunk
        for (int i = 0; i < CHUNK_SIZE - 1; i++) {
            for (int j = i + 1; j < CHUNK_SIZE; j++) {
                if (top_counts[i] < top_counts[j]) {
                    int temp = top_counts[i];
                    top_counts[i] = top_counts[j];
                    top_counts[j] = temp;
                }
            }
        }

        // Convert stay counts based on top K values
        for (int i = 0; i < CHUNK_SIZE; i++) {
            int count = 0;
            for (int j = 0; j < TOP_K && j < CHUNK_SIZE; j++) {
                if (stay_counts[chunk_start + i] == top_counts[j]) {
                    count++;
                }
            }
            converted_counts[i] = (count > 0) ? 1 : 0;
        }

        // Create binary string for the current chunk
        char binary_string[CHUNK_SIZE + 1];
        for (int i = 0; i < CHUNK_SIZE; i++) {
            binary_string[i] = converted_counts[i] + '0';
        }
        binary_string[CHUNK_SIZE] = '\0';

        // Map the binary string to a character in both directions for the first chunk
        if (!direction_determined) {
            char normal_char = map_binary_to_char(binary_string, false);
            char reverse_char = map_binary_to_char(binary_string, true);

            if (normal_char == '*') {
                *direction = false;
                printf("Direction determined: Normal\n");
            } else if (reverse_char == '*') {
                *direction = true;
                printf("Direction determined: Reverse\n");
            } else {
                printf("Error: Unable to determine direction from the first chunk.\n");
                return;
            }
        }

        // Map the binary string to a character based on the determined direction
        char mapped_char = map_binary_to_char(binary_string, *direction);
        printf("Mapped character from binary string '%s': %c\n", binary_string, mapped_char);

        // Store the mapped character
        if (char_index < CHAR_COUNT) {
            converted_chars[char_index++] = mapped_char;
        }
    }
}

// Add a flag to track if conversion has been done
bool conversion_done = false;

// Global or static variable to keep track of the last conversion barcount
int last_conversion_barcount = -1;

bool waiting_for_black = false;  // Flag to wait for black bar after every 9 bars

// Modified function to detect barcode with initial black detection and barcount printing

bool direction = false;  // false for normal, true for reverse

void barcode_detector(uint16_t analog_value) {
    bool current_state_black = (analog_value > BLACK_WHITE_THRESHOLD);

    // Check for transition only if we're not waiting for the next black bar
    if (!waiting_for_black && current_state_black != barcode_black_detected) {
        printf("Stayed %s for %d readings\n", 
               barcode_black_detected ? "black" : "white", 
               stay_count);

        // Only start counting when the first black is detected
        if (current_state_black && barcount == 0 && stay_count == 0) {
            printf("Starting barcode detection on first black detection.\n");
            stay_count = 1;  // Start counting the first black state
        } else if (stay_count > 0 && barcount < MAX_TRANSITIONS) {
            stay_counts[barcount] = stay_count;
            barcount++;
            printf("barcount updated to %d\n", barcount);  // Print barcount after increment
        }

        stay_count = 1; // Start counting the new state
        printf("Transition detected: %s to %s\n", 
               barcode_black_detected ? "black" : "white", 
               current_state_black ? "black" : "white");
    } else if (!waiting_for_black && stay_count > 0) {
        stay_count++;
    }

    barcode_black_detected = current_state_black;

    printf("Current analog value: %d, Current state: %s\n", 
           analog_value, 
           current_state_black ? "black" : "white");

    // Call convert_stay_counts only once at each multiple of 9 bars
    if (barcount % CHUNK_SIZE == 0 && barcount > 0 && barcount <= MAX_TRANSITIONS && barcount != last_conversion_barcount) {
        printf("Detected %d bars, converting to character...\n", barcount);
        if (!direction_determined) {
            convert_stay_counts(stay_counts + (barcount - CHUNK_SIZE), CHUNK_SIZE, &direction);
            direction_determined = true;
        } else {
            convert_stay_counts(stay_counts + (barcount - CHUNK_SIZE), CHUNK_SIZE, &direction);
        }
        last_conversion_barcount = barcount;  // Update last conversion barcount
        waiting_for_black = true;  // Set flag to wait for black bar
    }

    // Reset and print barcode after 36 transitions
    if (barcount >= MAX_TRANSITIONS) {
        printf("Finish detecting barcode with %d transitions\n", barcount);

        // Print all converted characters
        printf("Complete barcode: ");
        for (int i = 0; i < CHAR_COUNT; i++) {
            printf("%c", converted_chars[i]);
        }
        printf("\n");

        // Reset for next detection
        barcount = 0;
        char_index = 0;
        detect_barcode = false;
        last_conversion_barcount = -1;  // Reset last conversion barcount for next barcode
        waiting_for_black = false;  // Reset flag for the next barcode
        direction_determined = false;  // Reset direction determination for the next barcode
    }

    // Reset waiting_for_black if a black bar is detected
    if (waiting_for_black && current_state_black) {
        waiting_for_black = false;  // Reset to start counting after black bar is detected
        stay_count = 1;  // Start counting the new black bar
        printf("Detected black bar, resuming counting.\n");
    }
}

#define RESET_BUTTON_PIN 22 // Define the GPIO pin for the reset button

// Function prototype for reset
void reset_barcode_detector(uint gpio, uint32_t events);

// Function to initialize the button
void setup_button() {
    gpio_init(RESET_BUTTON_PIN);                 // Initialize the GPIO pin
    gpio_set_dir(RESET_BUTTON_PIN, GPIO_IN);      // Set it as an input
    gpio_pull_up(RESET_BUTTON_PIN);               // Enable pull-up resistor
    gpio_set_irq_enabled_with_callback(RESET_BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, &reset_barcode_detector);
}

// Reset function to clear barcode detector variables
void reset_barcode_detector(uint gpio, uint32_t events) {
    if (gpio == RESET_BUTTON_PIN) {
        // Reset barcode detection variables
                // Reset for next detection
        barcount = 0;
        char_index = 0;
        detect_barcode = false;
        last_conversion_barcount = -1;  // Reset last conversion barcount for next barcode
        waiting_for_black = false;  // Reset flag for the next barcode
        direction_determined = false;  // Reset direction determination for the next barcode
        stay_count = 0;
        barcode_black_detected = false;
        last_conversion_barcount = -1;

        // Clear the stay_counts array
        memset(stay_counts, 0, sizeof(stay_counts));

        // Notify reset
        printf("Barcode detector has been reset.\n");
    }
}

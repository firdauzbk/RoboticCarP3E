// #include <stdint.h>
// #include <stdbool.h>
// #include <stdio.h>
// #include "pico/stdlib.h"
// #include <string.h> 
// #include "hardware/adc.h"
// #include "hardware/gpio.h"
// #include "buddy3.h"
// #include "buddy2.h"

// #define LEFT_IR_SENSOR_ANALOG_PIN 26   // ADC GPIO pin for the left sensor
// #define RIGHT_IR_SENSOR_ANALOG_PIN 27   // ADC GPIO pin for the right sensor

// // Fixed threshold for surface detection
// const int BLACK_WHITE_THRESHOLD = 500;

// // Variables to track sensor states
// bool last_state_black[2] = {false, false}; // Last states for left and right
// bool detect_barcode = false;

// // Function prototypes
// void setup_adc();
// uint16_t read_adc(int sensor_index);
// void read_ir_sensors();
// void line_following(uint16_t analog_values[]);
// void print_detected_state(int sensor_index, bool current_state_black, uint16_t analog_value, float voltage);
// int find_binary_index(const char *binary_code, char *code_array[]);
// char map_binary_to_char(const char *binary_code, bool reverse);
// void barcode_detector(uint16_t analog_value);
// void convert_stay_counts(int stay_counts[], int barcount);

// // Function to set up ADC
// void setup_adc() {
//     adc_init();
//     adc_gpio_init(LEFT_IR_SENSOR_ANALOG_PIN);
//     adc_gpio_init(RIGHT_IR_SENSOR_ANALOG_PIN);
// }

// // Function to select ADC input based on sensor index
// uint16_t read_adc(int sensor_index) {
//     adc_select_input(sensor_index); // 0 for left, 1 for right
//     return adc_read(); // Read the selected ADC input
// }

// // Function to read IR sensors and print current state and values
// void read_ir_sensors() {
//     uint16_t analog_values[2];
//     float voltages[2];

//     // Read analog values from the sensors
//     for (int i = 0; i < 2; i++) {
//         analog_values[i] = read_adc(i); // Read from the corresponding sensor
//         voltages[i] = analog_values[i] * (3.3f / 4095.0f);  // Convert analog value to voltage
//         bool current_state_black = (analog_values[i] > BLACK_WHITE_THRESHOLD);
//         print_detected_state(i, current_state_black, analog_values[i], voltages[i]);
//         last_state_black[i] = current_state_black; // Update last state

//         // Start barcode detection
//         //if (i == 0 && current_state_black) {
//         //     detect_barcode = true;
//         // }

//         // if(i == 0 && detect_barcode){
//         //     barcode_detector(analog_values[i]);
//         // }

//         // Start line following
//         line_following(analog_values);
//     }
// }

// void print_detected_state(int sensor_index, bool current_state_black, uint16_t analog_value, float voltage) {
//     const char* sensor_name = (sensor_index == 0) ? "Barcode Detector Sensor" : "Line Following Sensor";

//     // Testing
//     if (sensor_index == 0) {
//         return;
//     }

//     if (current_state_black) {
//         printf("%s - Detected: Black Surface\n", sensor_name);
//     } else {
//         printf("%s - Detected: White Surface\n", sensor_name);
//     }

//     // Print the raw analog value and voltage for debugging
//     printf("%s - Analog Value: %u, Voltage: %.2fV\n", sensor_name, analog_value, voltage);
// }

// // Function to control line following based on right sensor
// void line_following(uint16_t analog_value[]) {
//     bool on_black = (analog_value[1] > BLACK_WHITE_THRESHOLD);

//     if (on_black) {
//         printf("On black surface. Moving forward...\n");
//         both_full_motor_forward();
//     } else {
//         printf("On white surface. Stopping...\n");
//         both_stop_motor();
//     }
// }

// // Initialise array used to store each barcode character
// char array_char[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
//                                 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
//                                 'Y', 'Z', '_', '.', '$', '/', '+', '%', ' ', '*'}; 

// // Initialise array used to store binary representation of each character
// char *array_code[] = {"000110100", "100100001", "001100001", "101100000", "000110001", "100110000", "001110000",
//                                 "000100101", "100100100", "001100100", "100001001", "001001001", "101001000", "000011001",
//                                 "100011000", "001011000", "000001101", "100001100", "001001100", "000011100", "100000011",
//                                 "001000011", "101000010", "000010011", "100010010", "001010010", "000000111", "100000110",
//                                 "001000110", "000010110", "110000001", "011000001", "111000000", "010010001", "110010000",
//                                 "011010000", "010000101", "110000100", "010101000", "010100010", "010001010", "000101010",
//                                 "011000100", "010010100"}; 

// // Initialise array used to store the reversed binary representation of each character
// char *array_reverse_code[] = {"001011000", "100001001", "100001100", "000001101", "100011000", "000011001",
//                                         "000011100", "101001000", "001001001", "001001100", "100100001", "100100100",
//                                         "000100101", "100110000", "000110001", "000110100", "101100000", "001100001",
//                                         "001100100", "001110000", "110000001", "110000100", "010000101", "110010000",
//                                         "010010001", "010010100", "111000000", "011000001", "011000100", "011010000",
//                                         "100000011", "100000110", "000000111", "100010010", "000010011", "000010110",
//                                         "101000010", "001000011", "000101010", "010001010", "010100010", "010101000",
//                                         "001000110", "001010010"};

// // Function to find the index of a binary code in the specified code array
// int find_binary_index(const char *binary_code, char *code_array[]) {
//     for (int i = 0; i < sizeof(array_code) / sizeof(array_code[0]); i++) {
//         if (strcmp(code_array[i], binary_code) == 0) {
//             return i; // Return index if binary code is found
//         }
//     }
//     return -1; // Return -1 if binary code is not found
// }

// // Function to map binary code to its corresponding character with an option for reverse mapping
// char map_binary_to_char(const char *binary_code, bool reverse) {
//     // Choose the appropriate array based on the reverse flag
//     char **code_array = reverse ? array_reverse_code : array_code;
    
//     int index = find_binary_index(binary_code, code_array); // Find the index in the selected array

//     if (index != -1) {
//         return array_char[index]; // Return the corresponding character
//     }
    
//     // Return 0 if the binary code is not found
//     return 0; 
// }

// #define MAX_TRANSITIONS 9 // Maximum number of transitions to track
// #define TOP_K 2 // Specify the value of k for conversion

// int barcount = 0;
// bool barcode_black_detected = false; // To track the last state
// int stay_count = 0; // Variable to track how long the sensor stays in the same state
// int stay_counts[MAX_TRANSITIONS]; // Array to store stay counts for each transition

// // Function to detect barcode
// void barcode_detector(uint16_t analog_value) {
//     // Determine the current state based on the analog value
//     bool current_state_black = (analog_value > BLACK_WHITE_THRESHOLD);
    
//     // Check for transition
//     if (current_state_black != barcode_black_detected) {
//         // Print how long it stayed in the previous state
//         printf("Stayed %s for %d readings\n", 
//                barcode_black_detected ? "black" : "white", 
//                stay_count);

//         // Store the stay count for this transition if within limits
//         if (barcount < MAX_TRANSITIONS) {
//             stay_counts[barcount] = stay_count; // Store the previous stay count
//         }
        
//         // Increment transition count and reset stay_count
//         barcount++;
//         stay_count = 1; // Reset stay count for the new state
//         printf("Transition detected: %s to %s\n", 
//                barcode_black_detected ? "black" : "white", 
//                current_state_black ? "black" : "white");
//     } else {
//         // If state remains the same, increment stay_count
//         stay_count++;
//     }

//     // Update the last state to the current state
//     barcode_black_detected = current_state_black;

//     // Optional: Print the current state for debugging
//     printf("Current analog value: %d, Current state: %s\n", 
//            analog_value, 
//            current_state_black ? "black" : "white");

//     // Finish detection after a certain number of transitions
//     if (barcount >= MAX_TRANSITIONS) {
//         printf("Finish detecting barcode with %d transitions\n", barcount);
        
//         // Print the stay counts for each transition
//         printf("Stay counts for each transition:\n");
//         for (int i = 0; i < barcount; i++) {
//             printf("Transition %d: %d readings\n", i + 1, stay_counts[i]);
//         }
        
//         // Convert stay counts based on max k value
//         convert_stay_counts(stay_counts, barcount);
        
//         // Reset detection
//         barcount = 0;
//         detect_barcode = false;
//     }
// }

// // Function to convert stay count based on max k value
// void convert_stay_counts(int stay_counts[], int barcount) {
//     // Create an array to store the converted values with original positions
//     int converted_counts[MAX_TRANSITIONS] = {0};
    
//     // Create an array to track the top K values
//     int top_counts[MAX_TRANSITIONS] = {0};

//     // Initialize the top_counts array to store the top K largest values
//     for (int i = 0; i < barcount; i++) {
//         top_counts[i] = stay_counts[i];
//     }

//     // Sort top_counts to find the top K values
//     for (int i = 0; i < barcount - 1; i++) {
//         for (int j = i + 1; j < barcount; j++) {
//             if (top_counts[i] < top_counts[j]) {
//                 int temp = top_counts[i];
//                 top_counts[i] = top_counts[j];
//                 top_counts[j] = temp;
//             }
//         }
//     }

//     // Convert stay counts based on the top K largest values
//     for (int i = 0; i < barcount; i++) {
//         int count = 0;
//         for (int j = 0; j < TOP_K; j++) {
//             if (stay_counts[i] == top_counts[j]) {
//                 count++;
//             }
//         }
//         // If current value is in the top K, set to 1, otherwise to 0
//         converted_counts[i] = (count > 0) ? 1 : 0;
//     }

//     // Print the converted stay counts
//     printf("Converted stay counts (top %d):\n", TOP_K);
//     for (int i = 0; i < barcount; i++) {
//         printf("Transition %d: %d\n", i + 1, converted_counts[i]);
//     }

//     // Map the converted array using map_binary_to_char
//     char binary_string[MAX_TRANSITIONS * 2 + 1]; // For storing the binary string
//     for (int i = 0; i < barcount; i++) {
//         binary_string[i] = converted_counts[i] + '0'; // Convert int to char
//     }
//     binary_string[barcount] = '\0'; // Null-terminate the string

//     // Use the mapping function (assuming you have defined array_code and array_char)
//     char mapped_char = map_binary_to_char(binary_string, false); // Use false for normal mapping
//     printf("Mapped character from binary string '%s': %c\n", binary_string, mapped_char);
// }


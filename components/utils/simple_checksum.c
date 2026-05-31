//
// Created by efisio on 5/31/26.
//

#include <stdio.h>
#include <stdbool.h>
#include "simple_checksum.h"


uint16_t calculate_checksum(uint8_t *data, size_t length) {
    // This function would compute the Simple Checksum value based on the input data
    printf("Data: ");
    for (size_t i = 0; i < sizeof(data); i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
    uint16_t initial_checksum = 0; // Example initial checksum value
    for (size_t i = 0; i < length; i++) {
        // Update the checksum
        initial_checksum += data[i] & 0xFF; // This is a placeholder for the actual checksum computation logic
    }
    return initial_checksum; // Store the computed checksum result
}

bool validate_checksum(uint8_t *data, size_t length, uint16_t expected_checksum) {
    // This function would validate the computed checksum against the expected value
    uint16_t computed_checksum = calculate_checksum(data, length);
    return (computed_checksum == expected_checksum); // Return true if the computed checksum matches the expected value
}
//
// Created by efisio on 5/31/26.
//

#ifndef CENZONTLE_SIMPLE_CHECKSUM_H
#define CENZONTLE_SIMPLE_CHECKSUM_H
#include <stdio.h>
#include <stdbool.h>

uint16_t calculate_checksum(uint8_t *data, size_t length);

bool validate_checksum(uint8_t *data, size_t length, uint16_t expected_checksum);

#endif //CENZONTLE_SIMPLE_CHECKSUM_H

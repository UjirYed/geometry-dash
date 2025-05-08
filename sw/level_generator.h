#ifndef _LEVEL_GENERATOR_H
#define _LEVEL_GENERATOR_H

#include <stdint.h>

// Generate a complete level
void generate_level(uint8_t* buffer, int level_length);

// Save level data to a file
void save_level_to_file(const char* filename, uint8_t* level_data, int level_length);

// Load level data from a file
int load_level_from_file(const char* filename, uint8_t* level_data, int max_length);

#endif // _LEVEL_GENERATOR_H 
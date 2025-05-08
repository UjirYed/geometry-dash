#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "geo_dash.h"

// Level generation parameters
#define MIN_GAP 5           // Minimum blocks between obstacles
#define MAX_GAP 10          // Maximum blocks between obstacles
#define LEVEL_HEIGHT 6      // Height of level in blocks
#define STARTING_BLOCKS 15  // Number of empty blocks at start
#define LEVEL_SECTIONS 5    // Number of difficulty sections

// Difficulty settings
typedef struct {
    int spike_chance;       // Chance of generating a spike (0-100)
    int block_chance;       // Chance of generating a block (0-100)
    int platform_chance;    // Chance of generating a platform (0-100)
    int jump_pad_chance;    // Chance of generating a jump pad (0-100)
    int portal_chance;      // Chance of generating a gravity portal (0-100)
    int min_gap;            // Minimum gap between obstacles
    int max_gap;            // Maximum gap between obstacles
} DifficultySettings;

// Level generator context
typedef struct {
    uint8_t* level_data;    // Buffer to store level data
    int level_length;       // Length of level in blocks
    int current_position;   // Current position in level
    DifficultySettings difficulty; // Current difficulty settings
} LevelGenerator;

// Predefined difficulty settings
DifficultySettings difficulties[LEVEL_SECTIONS] = {
    // Easy (tutorial)
    {
        .spike_chance = 10,
        .block_chance = 5,
        .platform_chance = 15,
        .jump_pad_chance = 0,
        .portal_chance = 0,
        .min_gap = 8,
        .max_gap = 15
    },
    // Medium
    {
        .spike_chance = 20,
        .block_chance = 10,
        .platform_chance = 15,
        .jump_pad_chance = 5,
        .portal_chance = 0,
        .min_gap = 6,
        .max_gap = 12
    },
    // Hard
    {
        .spike_chance = 25,
        .block_chance = 15,
        .platform_chance = 10,
        .jump_pad_chance = 10,
        .portal_chance = 5,
        .min_gap = 5,
        .max_gap = 10
    },
    // Very Hard
    {
        .spike_chance = 30,
        .block_chance = 20,
        .platform_chance = 10,
        .jump_pad_chance = 15,
        .portal_chance = 10,
        .min_gap = 4,
        .max_gap = 8
    },
    // Expert
    {
        .spike_chance = 35,
        .block_chance = 25,
        .platform_chance = 5,
        .jump_pad_chance = 20,
        .portal_chance = 15,
        .min_gap = 3,
        .max_gap = 6
    }
};

// Initialize the level generator
void init_level_generator(LevelGenerator* generator, uint8_t* buffer, int max_length) {
    generator->level_data = buffer;
    generator->level_length = max_length;
    generator->current_position = 0;
    generator->difficulty = difficulties[0]; // Start with easiest difficulty
    
    // Seed random number generator
    srand(time(NULL));
}

// Add empty space (gap) to the level
void add_empty_space(LevelGenerator* generator, int length) {
    for (int i = 0; i < length && generator->current_position < generator->level_length; i++) {
        generator->level_data[generator->current_position++] = OBS_NONE;
    }
}

// Add a specific obstacle to the level
void add_obstacle(LevelGenerator* generator, uint8_t obstacle_type) {
    if (generator->current_position < generator->level_length) {
        generator->level_data[generator->current_position++] = obstacle_type;
    }
}

// Add a random obstacle based on current difficulty
void add_random_obstacle(LevelGenerator* generator) {
    // Total chance for all obstacle types
    int total_chance = generator->difficulty.spike_chance + 
                      generator->difficulty.block_chance + 
                      generator->difficulty.platform_chance + 
                      generator->difficulty.jump_pad_chance + 
                      generator->difficulty.portal_chance;
    
    // If sum is 0, just add empty space
    if (total_chance == 0) {
        add_obstacle(generator, OBS_NONE);
        return;
    }
    
    // Select a random obstacle type
    int rand_val = rand() % total_chance;
    int cumulative = 0;
    
    // Determine which obstacle to add
    if ((cumulative += generator->difficulty.spike_chance) > rand_val) {
        add_obstacle(generator, OBS_SPIKE);
    } else if ((cumulative += generator->difficulty.block_chance) > rand_val) {
        add_obstacle(generator, OBS_BLOCK);
    } else if ((cumulative += generator->difficulty.platform_chance) > rand_val) {
        add_obstacle(generator, OBS_PLATFORM);
    } else if ((cumulative += generator->difficulty.jump_pad_chance) > rand_val) {
        add_obstacle(generator, OBS_JUMP_PAD);
    } else {
        add_obstacle(generator, OBS_GRAVITY_PORTAL);
    }
}

// Generate a pattern of obstacles (like a triple spike or stacked blocks)
void add_pattern(LevelGenerator* generator, int pattern_type) {
    switch (pattern_type) {
        case 0: // Double spike
            add_obstacle(generator, OBS_SPIKE);
            add_obstacle(generator, OBS_SPIKE);
            break;
            
        case 1: // Triple spike
            add_obstacle(generator, OBS_SPIKE);
            add_obstacle(generator, OBS_SPIKE);
            add_obstacle(generator, OBS_SPIKE);
            break;
            
        case 2: // Block with platform on top
            add_obstacle(generator, OBS_BLOCK);
            // In a full implementation, we'd place a platform above the block
            break;
            
        case 3: // Alternating spikes and blocks
            add_obstacle(generator, OBS_SPIKE);
            add_empty_space(generator, 1);
            add_obstacle(generator, OBS_BLOCK);
            add_empty_space(generator, 1);
            add_obstacle(generator, OBS_SPIKE);
            break;
            
        default:
            // Just add a single random obstacle
            add_random_obstacle(generator);
            break;
    }
}

// Generate a complete level
void generate_level(uint8_t* buffer, int level_length) {
    LevelGenerator generator;
    int section_length = level_length / LEVEL_SECTIONS;
    int current_section = 0;
    
    // Initialize generator
    init_level_generator(&generator, buffer, level_length);
    
    // Start with empty space
    add_empty_space(&generator, STARTING_BLOCKS);
    
    // Generate level sections with increasing difficulty
    while (generator.current_position < level_length) {
        // Update difficulty when entering a new section
        if (generator.current_position >= (current_section + 1) * section_length && 
            current_section < LEVEL_SECTIONS - 1) {
            current_section++;
            generator.difficulty = difficulties[current_section];
            
            // Add a "breather" gap when difficulty increases
            add_empty_space(&generator, generator.difficulty.max_gap);
        }
        
        // Decide whether to add a pattern or single obstacle (20% chance of pattern)
        if (rand() % 100 < 20) {
            add_pattern(&generator, rand() % 4);
        } else {
            add_random_obstacle(&generator);
        }
        
        // Add a gap between obstacles
        int gap_size = generator.difficulty.min_gap + 
                      rand() % (generator.difficulty.max_gap - generator.difficulty.min_gap + 1);
        add_empty_space(&generator, gap_size);
    }
}

// Save level data to a file
void save_level_to_file(const char* filename, uint8_t* level_data, int level_length) {
    FILE* file = fopen(filename, "w");
    if (file) {
        for (int i = 0; i < level_length; i++) {
            fprintf(file, "%d\n", level_data[i]);
        }
        fclose(file);
        printf("Level saved to %s\n", filename);
    } else {
        printf("Error: Could not save level to %s\n", filename);
    }
}

// Load level data from a file
int load_level_from_file(const char* filename, uint8_t* level_data, int max_length) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Could not load level from %s\n", filename);
        return 0;
    }
    
    int i = 0;
    int value;
    while (i < max_length && fscanf(file, "%d", &value) == 1) {
        level_data[i++] = (uint8_t)value;
    }
    
    fclose(file);
    printf("Loaded %d blocks from %s\n", i, filename);
    return i;
}

// Example usage
#ifdef TEST_LEVEL_GENERATOR
int main() {
    uint8_t level[MAX_LEVEL_LENGTH];
    
    // Generate a level
    generate_level(level, MAX_LEVEL_LENGTH);
    
    // Save it to a file
    save_level_to_file("level1.dat", level, MAX_LEVEL_LENGTH);
    
    return 0;
}
#endif 
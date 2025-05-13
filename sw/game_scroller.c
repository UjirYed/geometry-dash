#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include "geo_dash.h"

#define SCREEN_WIDTH 20
#define SCREEN_HEIGHT 15

// Level buffer - allocated dynamically based on level size
uint8_t *level_buffer = NULL;
int level_width = 0;

// Flag to control the scrolling loop
volatile int keep_running = 1;

/**
 * Signal handler for clean termination
 */
void handle_signal(int sig) {
    keep_running = 0;
}

/**
 * Load a level from a binary file
 * @param filename The level file path
 * @param width The width of the level (in tiles)
 * @return 0 on success, negative value on error
 */
int load_level(const char *filename, int width) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Error opening level file");
        return -1;
    }
    
    // Set the level width
    level_width = width;
    
    // Allocate memory for level buffer
    level_buffer = (uint8_t *)malloc(SCREEN_HEIGHT * level_width * sizeof(uint8_t));
    if (level_buffer == NULL) {
        perror("Failed to allocate memory for level");
        fclose(file);
        return -2;
    }
    
    // Initialize to zeros
    memset(level_buffer, 0, SCREEN_HEIGHT * level_width * sizeof(uint8_t));
    
    // Read the level data from file (row major format)
    size_t bytes_read = fread(level_buffer, 1, SCREEN_HEIGHT * level_width, file);
    printf("Read %zu bytes from level file\n", bytes_read);
    
    fclose(file);
    return 0;
}

/**
 * Clean up allocated memory for the level
 */
void cleanup_level() {
    if (level_buffer != NULL) {
        free(level_buffer);
        level_buffer = NULL;
    }
}

/**
 * Get tile value at specified row and column in the level
 */
uint8_t get_level_tile(int row, int col) {
    if (row < 0 || row >= SCREEN_HEIGHT || col < 0 || col >= level_width) {
        return 0; // Return empty tile for out of bounds
    }
    return level_buffer[row * level_width + col];
}

/**
 * Update the visible tilemap on the device
 * @param fd The device file descriptor
 * @param start_col The leftmost column of the level to display
 */
void update_screen(int fd, int start_col) {
    geo_dash_arg_t arg;
    
    // Write each visible tile to the device
    for (int row = 0; row < SCREEN_HEIGHT; row++) {
        for (int col = 0; col < SCREEN_WIDTH; col++) {
            // Get the tile value from our level buffer
            int level_col = start_col + col;
            
            // Get the tile from our level buffer
            uint8_t tile = get_level_tile(row, level_col);
            
            // Write the tile to the device
            arg.tilemap_row = row;
            arg.tilemap_col = col;
            arg.tile_value = tile;
            
            if (ioctl(fd, WRITE_TILE, &arg) < 0) {
                perror("Error writing tile");
                return;
            }
        }
    }
}

/**
 * Initialize the palette colors
 * @param fd The device file descriptor
 */
void initialize_palette(int fd) {
    geo_dash_arg_t arg;
    
    // Set up some basic colors
    uint32_t colors[] = {
        0x00000000,  // Color 0: Black/transparent
        0x00663300,  // Color 1: Brown (ground)
        0x00888888,  // Color 2: Gray (platforms)
        0x00FF0000,  // Color 3: Red (obstacles)
        0x00FFFFFF,  // Color 4: White (clouds)
        0x0000FF00,  // Color 5: Green
        0x000000FF,  // Color 6: Blue
        0x00FFFF00   // Color 7: Yellow
    };
    
    // Write the palette colors to the device
    for (int i = 0; i < 8; i++) {
        arg.color_index = i;
        arg.rgb = colors[i];
        
        if (ioctl(fd, WRITE_PALETTE, &arg) < 0) {
            perror("Error writing palette");
            return;
        }
    }
}

/**
 * Load a tileset from file
 * @param fd The device file descriptor
 * @param filename The tileset file path
 * @return 0 on success, negative value on error
 */
int load_tileset(int fd, const char *filename) {
    geo_dash_arg_t arg;
    
    // Initialize tileset with zeros
    for (int row = 0; row < 32; row++) {
        for (int col = 0; col < 32; col++) {
            arg.tileset[row][col] = 0;
        }
    }
    
    // Read tileset from file
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Error opening tileset file");
        return -1;
    }
    int i = 0;
    // Read the file byte by byte and populate the tileset
    for (int row = 0; row < 32; row++) {
        for (int col = 0; col < 32; col++) {
            uint8_t byte;
            if (fread(&byte, 1, 1, file) != 1) {
                // If we reach end of file before filling the array,
                // fill remaining with zeros
                if (feof(file)) {
                    arg.tileset[row][col] = 0;
                    continue;
                } else {
                    perror("Error reading from file");
                    fclose(file);
                    return -2;
                }
            }
            arg.tileset[row][col] = byte;
        }
    }
    
    fclose(file);
    
    // Write the tileset to the device
    arg.tile_no = i;
    if (ioctl(fd, WRITE_TILESET, &arg) < 0) {
        perror("Error writing tileset");
        return -3;
    }
    
    printf("Tileset successfully loaded and written to device\n");
    return 0;
}

int main(int argc, char *argv[]) {
    int fd;
    int scroll_position = 0;
    struct timespec sleep_time;
    
    // Check for required arguments
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <tileset_filename> <level_filename> <level_width> [scroll_delay_ms]\n", argv[0]);
        fprintf(stderr, "  tileset_filename: Binary file containing tileset data\n");
        fprintf(stderr, "  level_filename: Binary file containing level data in row-major format\n");
        fprintf(stderr, "  level_width: Width of the level in tiles\n");
        fprintf(stderr, "  scroll_delay_ms: Optional delay between scrolls in milliseconds (default: 1000)\n");
        return -1;
    }
    
    // Parse the level width
    level_width = atoi(argv[3]);
    if (level_width <= 0) {
        fprintf(stderr, "Error: Level width must be a positive integer\n");
        return -1;
    }
    
    // Parse optional scroll delay (default: 1000ms = 1 second)
    int scroll_delay_ms = 1000;
    if (argc > 4) {
        scroll_delay_ms = atoi(argv[4]);
        if (scroll_delay_ms < 0) {
            fprintf(stderr, "Warning: Invalid scroll delay, using default (1000ms)\n");
            scroll_delay_ms = 1000;
        }
    }
    
    // Set up signal handlers for clean termination
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    // Open the geo_dash device
    fd = open("/dev/geo_dash", O_RDWR);
    if (fd < 0) {
        perror("Error opening device");
        return -1;
    }
    
    // Load the tileset
    if (load_tileset(fd, argv[1]) < 0) {
        fprintf(stderr, "Failed to load tileset\n");
        close(fd);
        return -1;
    }
    
    // Initialize the palette
    initialize_palette(fd);
    
    // Load the level
    if (load_level(argv[2], level_width) < 0) {
        fprintf(stderr, "Failed to load level\n");
        close(fd);
        return -1;
    }
    
    printf("Starting level scrolling. Press Ctrl+C to exit.\n");
    printf("Level width: %d tiles\n", level_width);
    printf("Scroll delay: %d ms\n", scroll_delay_ms);
    
    // Main scrolling loop
    while (keep_running && scroll_position < level_width - SCREEN_WIDTH) {
        // Update the screen with the current scroll position
        update_screen(fd, scroll_position);
        
        // Display some debug info
        printf("Scroll position: %d/%d\r", scroll_position, level_width - SCREEN_WIDTH);
        fflush(stdout);
        
        // Wait for specified delay
        sleep_time.tv_sec = scroll_delay_ms / 1000;
        sleep_time.tv_nsec = (scroll_delay_ms % 1000) * 1000000; // Convert to nanoseconds
        nanosleep(&sleep_time, NULL);
        
        // Increment scroll position
        scroll_position++;
    }
    
    printf("\nScrolling complete!\n");
    
    // Clean up
    cleanup_level();
    close(fd);
    return 0;
}

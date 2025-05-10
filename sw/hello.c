#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <time.h>
#include "geo_dash.h"

/**
 * Reads a tileset from a binary file into the tileset array
 * 
 * @param filename The path to the binary file
 * @param tileset The 32x32 tileset array to populate
 * @return 0 on success, negative value on error
 */
int read_tileset_from_file(const char *filename, uint8_t tileset[32][32]) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Error opening tileset file");
        return -1;
    }
    
    // Read the file byte by byte and populate the tileset
    for (int row = 0; row < 32; row++) {
        for (int col = 0; col < 32; col++) {
            uint8_t byte;
            if (fread(&byte, 1, 1, file) != 1) {
                // If we reach end of file before filling the array,
                // fill remaining with zeros
                if (feof(file)) {
                    tileset[row][col] = 0;
                    continue;
                } else {
                    perror("Error reading from file");
                    fclose(file);
                    return -2;
                }
            }
            tileset[row][col] = byte;
        }
    }
    
    fclose(file);
    return 0;
}

/**
 * Prints the tileset content (useful for debugging)
 */
void print_tileset(uint8_t tileset[32][32]) {
    printf("Tileset content (first 10x10):\n");
    for (int row = 0; row < 10; row++) {
        for (int col = 0; col < 10; col++) {
            printf("%3d ", tileset[row][col]);
        }
        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    geo_dash_arg_t arg;
    int fd;
    
    // Check if a tileset filename was provided
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <tileset_filename>\n", argv[0]);
        return -1;
    }
    
    // Initialize tileset with zeros
    for (int row = 0; row < 32; row++) {
        for (int col = 0; col < 32; col++) {
            arg.tileset[row][col] = 0;
        }
    }
    
    // Read tileset from the specified file
    int result = read_tileset_from_file(argv[1], arg.tileset);
    if (result < 0) {
        fprintf(stderr, "Failed to read tileset file\n");
        return -1;
    }
    
    // Optional: Print first part of the tileset to verify
    print_tileset(arg.tileset);
    
    // Open the geo_dash device
    fd = open("/dev/geo_dash", O_RDWR);
    if (fd < 0) {
        perror("Error opening device");
        return -1;
    }
    
    // Set tile
    arg.tilemap_col = 0;
    arg.tilemap_row = 0;
    arg.tile_value = 0;
    if (ioctl(fd, WRITE_TILE, &arg) < 0) {
        perror("Error writing tile");
        close(fd);
        return -1;
    }
    
    // Set palette
    arg.rgb = 0xff000000; // Red color
    for (int i = 0; i < 8; i++) {
        arg.color_index = i;
        printf("calling ioctl]\n");
        if (ioctl(fd, WRITE_PALETTE, &arg) < 0) {
            perror("Error writing palette");
            close(fd);
            return -1;
        }
    }
    
    // Write the tileset to the device
    arg.tile_no = 0;
    if (ioctl(fd, WRITE_TILESET, &arg) < 0) {
        perror("Error writing tileset");
        close(fd);
        return -1;
    }
    
    printf("Tileset successfully loaded and written to device\n");
    
    close(fd);
    return 0;
}

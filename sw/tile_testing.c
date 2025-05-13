#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <time.h>
#include <string.h>
#include "geo_dash.h"

#define TILE_WIDTH 32 // 32 pixels
#define TILE_HEIGHT 32 // 32 pixels
#define TILE_SIZE (TILE_WIDTH * TILE_HEIGHT)
int fill_screen(int fd, uint8_t tile_id);

/**
 * Fill the entire visible screen with a single tile value
 * @param fd The device file descriptor
 * @param tile_id The tile ID to fill the screen with
 * @return 0 on success, negative on failure
 */
int fill_screen(int fd, uint8_t tile_id) {
    geo_dash_arg_t arg;
    
    // Iterate through all visible tiles
    for (int row = 0; row < SCREEN_HEIGHT; row++) {
        for (int col = 0; col < SCREEN_WIDTH; col++) {
            // Set up arguments
            arg.tilemap_row = row;
            arg.tilemap_col = col;
            arg.tile_value = tile_id;
            
            // Write the tile
            if (ioctl(fd, WRITE_TILE, &arg) < 0) {
                perror("Error writing tile in fill_screen");
                return -1;
            }
        }
    }
    
    printf("Screen filled with tile ID: %d\n", tile_id);
    return 0;
}

int read_tile(FILE *file, uint8_t tileset[TILE_HEIGHT][TILE_WIDTH]) {
    for (int row = 0; row < TILE_HEIGHT; row++) {
        for (int col = 0; col < TILE_WIDTH; col++) {
            uint8_t byte;
            if (fread(&byte, 1, 1, file) != 1) {
                if (feof(file)) return 1; // Signal EOF after partial read
                perror("Error reading from file");
                return -1;
            }
            tileset[row][col] = byte;
        }
    }
    return 0;
}

void print_tile(uint8_t tileset[TILE_HEIGHT][TILE_WIDTH]) {
    for (int row = 0; row < 10; row++) {
        for (int col = 0; col < 10; col++) {
            printf("%3d ", tileset[row][col]);
        }
        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    geo_dash_arg_t arg;
    FILE *file;
    int fd, tile_no = 0;

    

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <tileset_filename>\n", argv[0]);
        return -1;
    }

    file = fopen(argv[1], "rb");
    if (!file) {
        perror("Error opening tileset file");
        return -1;
    }

    fd = open("/dev/geo_dash", O_RDWR);
    if (fd < 0) {
        perror("Error opening device");
        fclose(file);
        return -1;
    }

    fill_screen()
    while (1) {
        memset(&arg, 0, sizeof(geo_dash_arg_t));
        int result = read_tile(file, arg.tileset);
        if (result == 1) {
            // Reached end of file
            break;
        } else if (result < 0) {
            // Error
            fclose(file);
            close(fd);
            return -1;
        }

        arg.tile_no = tile_no++;
        if (tile_no < 5) { // Optional: print first few for debug
            printf("Loaded tile #%d:\n", arg.tile_no);
            print_tile(arg.tileset);
        }

        if (ioctl(fd, WRITE_TILESET, &arg) < 0) {
            perror("Error writing tileset");
            fclose(file);
            close(fd);
            return -1;
        }
    }

    printf("All %d tiles successfully written to device.\n", tile_no);

    fclose(file);
    close(fd);
    return 0;
}

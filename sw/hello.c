#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <time.h>
#include <string.h>
#include "geo_dash.h"

/**
 * Reads a tileset from a binary file into the tileset array
 * 
 * @param filename The path to the binary file
 * @param tileset The 32x32 tileset array to populate
 * @return 0 on success, negative value on error
 */

int ret;
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
    /* Populate the tile map. */
    arg.tilemap_col = 0;
    arg.tilemap_row = 1;

    for (int i = 0; i < 10; i++) {
        arg.tilemap_col = i;
        arg.tile_value = i;
        printf("writing tile id %d to tile map.\n", arg.tile_value);
        if (ioctl(fd, WRITE_TILE, &arg) < 0) {
            perror("Error writing tile");
            close(fd);
            return -1;
        }
    }
    //memset(&arg, 0, sizeof(geo_dash_arg_t));
    arg.tilemap_row = 0;
    arg.tilemap_col = 0;
    ret = ioctl(fd, READ_TILE, &arg);
    if (ret < 0) {
        perror("READ_TILE failed");
    }

    printf("the tile value at 0,0 is: %d\n", arg.tile_value);

    // Set palette
    arg.rgb = 0x00ff0000; // Red color
    arg.color_index = 0;
    if (ioctl(fd, WRITE_PALETTE, &arg) < 0) {
        perror("Error writing palette");
        close(fd);
        return -1;
    }
    arg.rgb = 0x0000ff00; // Red color
    arg.color_index = 6;
    if (ioctl(fd, WRITE_PALETTE, &arg) < 0) {
        perror("Error writing palette");
        close(fd);
        return -1;
    }
    arg.rgb = 0x000000ff; // blue color
    arg.color_index = 7;
    if (ioctl(fd, WRITE_PALETTE, &arg) < 0) {
        perror("Error writing palette");
    // Write the tileset to the device
    }
    arg.tile_no = 0;
    if (ioctl(fd, WRITE_TILESET, &arg) < 0) {
        perror("Error writing tileset");
        close(fd);
        return -1;
    }
    
    printf("Tileset successfully loaded and written to device\n");
    printf("Writing scroll offset\n");
    memset(&arg, 0, sizeof(arg));

	arg.player_y = (uint8_t) 255;
	if (ioctl(fd, WRITE_PLAYER_Y_POS, &arg) < 0) {
		perror("Error writing player Y position");
		close(fd);
		return -1;
	}

    // scroll from 0 to 1279 pixels, then wrap
    for (unsigned short offs = 0; ; offs = (offs + 4) % (40*32)) {
        arg.scroll_offset = offs;
        if (ioctl(fd, WRITE_SCROLL_OFFSET, &arg) < 0) {
            perror("Error writing scroll offset");
            break;
        }
        usleep(16000);   // ~60 Hz animation
    }
    
    close(fd);
    return 0;
}

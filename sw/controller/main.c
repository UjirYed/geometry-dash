#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "usbjoypad.h"
#include <pthread.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include "../geo_dash.h"   // Include the kernel module header

#define DEVICE_PATH "/dev/player_sprite_0"

int main() {
    controller_init();
    
    // Open the kernel module device file
    int fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open the device");
        fprintf(stderr, "Make sure the kernel module is loaded and /dev/player_sprite_0 exists\n");
        exit(EXIT_FAILURE);
    }
    
    printf("Controller initialized. Connected to game device.\n");
    printf("Controls: a=Left, d=Right, w/space=Jump, s=Start/Pause\n");

    // Game state
    unsigned short player_y = 240;  // Default Y position
    unsigned short x_shift = 0;     // Initial X position
    uint8_t flags = 0;              // Game flags
    
    while (true) {
        ControllerState state = controller_get_state();
        geo_dash_arg_t args;
        
        // Update game flags based on controller state
        flags = 0;
        if (state.buttonAPressed) {
            flags |= PLAYER_JUMPING;
        }
        
        // Send flags to kernel module
        args.flags = flags;
        if (ioctl(fd, WRITE_FLAGS, &args) < 0) {
            perror("Error sending flags to kernel module");
        }
        
        // Handle left/right movement by updating x_shift
        if (state.leftArrowPressed) {
            x_shift = (x_shift > 0) ? x_shift - 1 : 0;
        }
        if (state.rightArrowPressed) {
            x_shift++;
        }
        
        // Send x_shift to kernel module
        args.x_shift = x_shift;
        if (ioctl(fd, WRITE_X_SHIFT, &args) < 0) {
            perror("Error sending x_shift to kernel module");
        }
        
        // Handle jumping by adjusting player_y
        if (state.buttonAPressed) {
            player_y -= 10;  // Move up when jumping
        } else {
            player_y += 5;   // Gravity - move down when not jumping
            if (player_y > 240) {
                player_y = 240;  // Don't go below the ground
            }
        }
        
        // Send player_y to kernel module
        args.player_y = player_y;
        if (ioctl(fd, WRITE_PLAYER_Y_POS, &args) < 0) {
            perror("Error sending player_y to kernel module");
        }
        
        // Print current state
        printf("Left: %s | Right: %s | A: %s | Start: %s | Position: x=%d, y=%d\n",
               state.leftArrowPressed ? "Pressed" : "Released",
               state.rightArrowPressed ? "Pressed" : "Released",
               state.buttonAPressed ? "Pressed" : "Released",
               state.startPressed ? "Pressed" : "Released",
               x_shift, player_y);
        
        usleep(10000); // 10ms delay
    }

    close(fd);
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <time.h>
#include "geo_dash.h"
#include "level_generator.h"

// Game states
#define LOADING 2
#define READY 4
#define PLAYING 6
#define GAME_OVER 8

// Game constants
#define GROUND_Y 220          // Ground position (higher number = lower on screen)
#define PLAYER_SPEED 2        // Horizontal movement speed
#define JUMP_VELOCITY 10      // Initial jump velocity
#define GRAVITY 1             // Gravity acceleration
#define BLOCK_SIZE 32         // Size of a block in pixels
#define SCREEN_COLS 20        // Number of columns on screen (640/32 = 20)
#define DISPLAY_HEIGHT 15     // Height of level in blocks (480/32 = 15)
#define DISPLAY_WIDTH 128     // Width of level buffer
#define PLAYER_X 80           // Fixed player X position on screen
#define LEVEL_LENGTH 1024     // Length of the level in blocks
#define TILEMAP_WIDTH 40      // Width of double-screen tilemap (SCREEN_COLS * 2)
#define MAX_SCROLL_OFFSET 1280 // Maximum scroll offset (TILEMAP_WIDTH * BLOCK_SIZE)

typedef struct {
    int x_pos;                // Position in the level (pixels)
    int y_pos;                // Position on screen (pixels)
    int y_vel;                // Vertical velocity (pixels/frame)
    int is_jumping;           // Whether player is jumping
    int is_dead;              // Whether player is dead
    int is_gravity_inverted;  // Whether gravity is inverted
} Player;

// Global variables
Player player;
int button_pressed = 0;       // Input from button
int x_shift = 0;              // Pixel shift for scrolling
int level_position = 0;       // Current position in level
int score = 0;                // Player score
int fd;                       // File descriptor for device
int gravity_direction = 1;    // 1 for normal, -1 for inverted
int scroll_offset = 0;        // Tile scrolling offset (0-1279 pixels)

// Level data
uint8_t level_buf[LEVEL_LENGTH];   // Level data buffer
uint8_t disp_buf[TILEMAP_WIDTH];   // Display buffer (double-width for scrolling)

// Function prototypes
int loadMapAndMusic(void);
int runGamePhysics(void);
void updateDisplay(void);
int getUserInput(void);
void startAudioPlayback(void);
void initializeScrollingTilemap(void);
void updateScrollingTilemap(void);
void checkCollisions(void);
void initializeGame(void);
void gameOver(void);
void handleObstacleEffect(uint8_t obstacle_type);

// Example main function to show continuous scrolling
int main() {
    // Open the driver
    fd = open("/dev/player_sprite_0", O_RDWR);
    if (fd < 0) {
        perror("Error opening device");
        return -1;
    }

    // Initialize the game
    initializeGame();
    
    // Main game loop
    while (1) {
        // Get user input
        button_pressed = getUserInput();
        
        // Run physics
        runGamePhysics();
        
        // Update game display
        updateDisplay();
        
        // Sleep a bit (60 FPS approx)
        usleep(16667);
    }
    
    close(fd);
    return 0;
}

void initializeGame() {
    // Initialize player
    player.x_pos = 0;
    player.y_pos = GROUND_Y;
    player.y_vel = 0;
    player.is_jumping = 0;
    player.is_dead = 0;
    player.is_gravity_inverted = 0;
    
    // Reset game variables
    x_shift = 0;
    level_position = 0;
    score = 0;
    gravity_direction = 1;
    
    // Generate a new level
    generate_level(level_buf, LEVEL_LENGTH);
    
    // Initialize the double-width scrolling tilemap
    initializeScrollingTilemap();
    
    // Reset display
    updateDisplay();
}

// Initialize the double-width scrolling tilemap with the initial level data
void initializeScrollingTilemap() {
    // Fill the first screen with level data
    for (int i = 0; i < SCREEN_COLS; i++) {
        disp_buf[i] = level_buf[i];
    }
    
    // Fill the second screen with the next part of the level
    for (int i = 0; i < SCREEN_COLS; i++) {
        disp_buf[i + SCREEN_COLS] = level_buf[i + SCREEN_COLS];
    }
    
    // Initialize scroll offset to 0
    scroll_offset = 0;
    
    // Send initial scroll offset to hardware
    geo_dash_arg_t arg;
    arg.scroll_offset = scroll_offset;
    ioctl(fd, WRITE_SCROLL_OFFSET, &arg);
}

// Update the scrolling tilemap when the player moves
void updateScrollingTilemap() {
    // Calculate which screen we're currently displaying
    int current_screen = scroll_offset / (SCREEN_COLS * BLOCK_SIZE);
    
    // When we've scrolled half a screen width, update the content of the offscreen part
    if (scroll_offset % (SCREEN_COLS * BLOCK_SIZE) == 0 && scroll_offset > 0) {
        int update_start;
        int level_start;
        
        if (current_screen == 0) {
            // We're viewing the first screen, update the second screen
            update_start = SCREEN_COLS;
            level_start = level_position / BLOCK_SIZE + SCREEN_COLS * 2;
        } else {
            // We're viewing the second screen, update the first screen
            update_start = 0;
            level_start = level_position / BLOCK_SIZE + SCREEN_COLS * 2;
        }
        
        // Update tiles in the offscreen area
        for (int i = 0; i < SCREEN_COLS; i++) {
            int level_idx = level_start + i;
            if (level_idx < LEVEL_LENGTH) {
                disp_buf[update_start + i] = level_buf[level_idx];
            } else {
                disp_buf[update_start + i] = 0; // Empty tile if beyond level length
            }
        }
        
        // Write the updated tilemap to hardware
        // In a real implementation, you would update the tilemap memory through the Avalon interface
        // This is simplified for illustration
    }
}

int loadMapAndMusic() {
    // Set the background to the initial color
    geo_dash_arg_t arg;
    arg.bg_r = 50;
    arg.bg_g = 100;
    arg.bg_b = 200;
    ioctl(fd, WRITE_BACKGROUND_R, &arg);
    ioctl(fd, WRITE_BACKGROUND_G, &arg);
    ioctl(fd, WRITE_BACKGROUND_B, &arg);
    
    return 1; // Successfully loaded
}

int getUserInput() {
    // In a real implementation, this would read from hardware button
    // For simulation, let's read from keyboard
    char input;
    fd_set set;
    struct timeval timeout;
    
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    
    if (select(STDIN_FILENO + 1, &set, NULL, NULL, &timeout) > 0) {
        read(STDIN_FILENO, &input, 1);
        if (input == ' ') {
            return 1; // Button pressed
        }
    }
    
    return 0; // Button not pressed
}

int runGamePhysics() {
    // Jump when button is pressed and player is on ground
    if (button_pressed && !player.is_jumping) {
        player.y_vel = -JUMP_VELOCITY * gravity_direction;
        player.is_jumping = 1;
    }
    
    // Apply gravity
    player.y_vel += GRAVITY * gravity_direction;
    
    // Update player position
    player.y_pos += player.y_vel;
    
    // Check if player has landed on ground (depends on gravity direction)
    if (gravity_direction > 0) {
        // Normal gravity
        if (player.y_pos >= GROUND_Y) {
            player.y_pos = GROUND_Y;
            player.y_vel = 0;
            player.is_jumping = 0;
        }
    } else {
        // Inverted gravity
        if (player.y_pos <= 50) { // Top of the screen
            player.y_pos = 50;
            player.y_vel = 0;
            player.is_jumping = 0;
        }
    }
    
    // Move the level (player stays in fixed position)
    player.x_pos += PLAYER_SPEED;
    level_position += PLAYER_SPEED;
    
    // Update scroll offset for the double-width buffer
    scroll_offset = (scroll_offset + PLAYER_SPEED) % MAX_SCROLL_OFFSET;
    
    // Check if we need to update the tilemap content for continuous scrolling
    updateScrollingTilemap();
    
    return 1;
}

void handleObstacleEffect(uint8_t obstacle_type) {
    switch (obstacle_type) {
        case OBS_JUMP_PAD:
            // Extra boost jump
            player.y_vel = -JUMP_VELOCITY * 1.5 * gravity_direction;
            player.is_jumping = 1;
            break;
            
        case OBS_GRAVITY_PORTAL:
            // Invert gravity
            gravity_direction *= -1;
            player.is_gravity_inverted = !player.is_gravity_inverted;
            break;
    }
}

void checkCollisions() {
    // Get the block at player's position
    int player_block_x = (player.x_pos + PLAYER_X) / BLOCK_SIZE;
    
    // Check for collision with obstacles
    if (player_block_x < DISPLAY_WIDTH) {
        uint8_t block_type = disp_buf[player_block_x];
        
        switch (block_type) {
            case OBS_SPIKE:
                // Die on spike collision
                if (player.y_pos + BLOCK_SIZE/2 > GROUND_Y - BLOCK_SIZE/2) {
                    player.is_dead = 1;
                    printf("Hit spike! Game over.\n");
                }
                break;
                
            case OBS_BLOCK:
                // Die on block collision
                if (player.y_pos + BLOCK_SIZE/2 > GROUND_Y - BLOCK_SIZE) {
                    player.is_dead = 1;
                    printf("Hit block! Game over.\n");
                }
                break;
                
            case OBS_PLATFORM:
                // Land on platform if falling
                if (gravity_direction > 0 && player.y_vel > 0 && 
                    player.y_pos < GROUND_Y - BLOCK_SIZE) {
                    player.y_pos = GROUND_Y - BLOCK_SIZE;
                    player.y_vel = 0;
                    player.is_jumping = 0;
                }
                break;
                
            case OBS_JUMP_PAD:
            case OBS_GRAVITY_PORTAL:
                // Handle special obstacles
                handleObstacleEffect(block_type);
                break;
        }
    }
    
    // Check if player went off screen
    if ((gravity_direction > 0 && player.y_pos < 0) ||
        (gravity_direction < 0 && player.y_pos > 300)) {
        player.is_dead = 1;
        printf("Went off screen! Game over.\n");
    }
}

void updateDisplay() {
    // Update hardware with current game state
    geo_dash_arg_t arg;
    
    // Update player position
    arg.player_y = player.y_pos;
    ioctl(fd, WRITE_PLAYER_Y_POS, &arg);
    
    // Update x shift for scrolling (fine pixel adjustment)
    arg.x_shift = x_shift;
    ioctl(fd, WRITE_X_SHIFT, &arg);
    
    // Update the scroll offset for the double-width tilemap
    arg.scroll_offset = scroll_offset;
    ioctl(fd, WRITE_SCROLL_OFFSET, &arg);
    
    // Update map block (assuming this controls which part of the level is shown)
    arg.map_block = level_position / BLOCK_SIZE;
    ioctl(fd, WRITE_MAP_BLOCK, &arg);
    
    // Set background color based on current section of the level
    // This creates a nice color transition as the player progresses
    int level_progress = (level_position * 100) / (LEVEL_LENGTH * BLOCK_SIZE);
    
    arg.bg_r = 50 + (level_progress * 150) / 100;
    arg.bg_g = 100 + (level_progress * 50) / 100;
    arg.bg_b = 200 - (level_progress * 100) / 100;
    
    // Keep values in range
    if (arg.bg_r > 255) arg.bg_r = 255;
    if (arg.bg_g > 255) arg.bg_g = 255;
    if (arg.bg_b > 255) arg.bg_b = 255;
    
    ioctl(fd, WRITE_BACKGROUND_R, &arg);
    ioctl(fd, WRITE_BACKGROUND_G, &arg);
    ioctl(fd, WRITE_BACKGROUND_B, &arg);
    
    // Set flags based on game state
    arg.flags = 0;
    if (player.is_jumping) arg.flags |= PLAYER_JUMPING;
    if (player.is_dead) arg.flags |= PLAYER_DEAD;
    if (player.is_gravity_inverted) arg.flags |= PLAYER_INVERTED;
    
    ioctl(fd, WRITE_FLAGS, &arg);
    
    // Output flags can be used to indicate game state to the hardware
    arg.output_flags = score / 1000; // Just an example
    ioctl(fd, WRITE_OUTPUT_FLAGS, &arg);
}

void startAudioPlayback() {
    // Start the background music
    // In a real implementation, this would communicate with audio hardware
    printf("Starting audio playback...\n");
}

void gameOver() {
    // Handle game over state
    printf("Game Over! Final score: %d\n", score);
    printf("Press button to restart\n");
    
    // Save high score if needed
    // This would normally write to a file
}




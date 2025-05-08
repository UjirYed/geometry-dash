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
#define SCREEN_COLS 20        // Number of columns on screen
#define DISPLAY_HEIGHT 6      // Height of level in blocks
#define DISPLAY_WIDTH 128     // Width of level buffer
#define PLAYER_X 80           // Fixed player X position on screen
#define LEVEL_LENGTH 1024     // Length of the level in blocks

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

// Level data
uint8_t level_buf[LEVEL_LENGTH];   // Level data buffer
uint8_t disp_buf[DISPLAY_WIDTH];   // Display buffer

// Function prototypes
int loadMapAndMusic(void);
int runGamePhysics(void);
void updateDisplay(void);
int getUserInput(void);
void startAudioPlayback(void);
void copyNextColumn(void);
void checkCollisions(void);
void initializeGame(void);
void gameOver(void);
void handleObstacleEffect(uint8_t obstacle_type);

int main() {
    int current_state = LOADING;
    
    // Open the device file
    fd = open("/dev/player_sprite_0", O_RDWR);
    if (fd == -1) {
        perror("Error opening device");
        return -1;
    }
    
    // Seed random number generator
    srand(time(NULL));
    
    initializeGame();
    
    while (1) {
        // Get user input
        button_pressed = getUserInput();
        
        switch (current_state) {
            case LOADING:
                if (loadMapAndMusic()) {
                    current_state = READY;
                    printf("Game ready! Press button to start.\n");
                }
                break;
                
            case READY:
                if (button_pressed) {
                    current_state = PLAYING;
                    startAudioPlayback();
                    printf("Game started!\n");
                }
                break;
                
            case PLAYING:
                runGamePhysics();
                checkCollisions();
                updateDisplay();
                
                // Check if player died
                if (player.is_dead) {
                    current_state = GAME_OVER;
                    gameOver();
                }
                
                // Increment score based on distance traveled
                score += PLAYER_SPEED;
                break;
                
            case GAME_OVER:
                if (button_pressed) {
                    // Reset game
                    initializeGame();
                    current_state = READY;
                    printf("Game reset! Press button to start.\n");
                }
                break;
        }
        
        // Small delay to control game speed
        usleep(16667); // ~60 FPS
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
    
    // Reset display
    updateDisplay();
}

int loadMapAndMusic() {
    // Initialize display buffer
    for (int i = 0; i < DISPLAY_WIDTH; i++) {
        disp_buf[i] = level_buf[i];
    }
    
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
    x_shift += PLAYER_SPEED;
    
    // If we've shifted by a full block, update the display buffer
    if (x_shift >= BLOCK_SIZE) {
        x_shift -= BLOCK_SIZE;
        copyNextColumn();
    }
    
    return 1;
}

void copyNextColumn() {
    // Shift all columns left
    for (int i = 0; i < DISPLAY_WIDTH - 1; i++) {
        disp_buf[i] = disp_buf[i + 1];
    }
    
    // Add new column from level data
    int next_block = level_position / BLOCK_SIZE + DISPLAY_WIDTH;
    if (next_block < LEVEL_LENGTH) {
        disp_buf[DISPLAY_WIDTH - 1] = level_buf[next_block];
    } else {
        disp_buf[DISPLAY_WIDTH - 1] = EMPTY; // End of level
    }
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
    
    // Update x shift for scrolling
    arg.x_shift = x_shift;
    ioctl(fd, WRITE_X_SHIFT, &arg);
    
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




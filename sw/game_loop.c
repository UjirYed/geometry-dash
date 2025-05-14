#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <pthread.h>
#include "geo_dash.h"
#include "../controller/usbjoypad.h"

#define SCREEN_WIDTH 20
#define SCREEN_HEIGHT 15
#define TILE_HEIGHT 32

// Player sprite constants
#define PLAYER_TILE 8             // Tile index for player sprite
#define PLAYER_START_X 10          // Starting X position (screen coordinate)
#define PLAYER_START_Y 11         // Starting Y position (screen coordinate)
#define GRAVITY 2              // Gravity force
#define JUMP_VELOCITY -20        // Initial jump velocity (negative means upward)
#define MAX_FALL_SPEED 5.0        // Maximum falling speed
#define GROUND_Y 11               // Ground Y position
#define REG_GROUND 255               // Ground Y position

#define GAMEOVER_BIN  "../hw/gd-tiles/gameover.bin"

// Game state
typedef struct {
    float player_x;               // Player X position in screen coordinates
    float player_y;               // Player Y position in screen coordinates
    float player_vy;              // Player vertical velocity
    bool is_jumping;              // Is the player currently jumping?
    bool is_dead;                 // Is the player dead?
    int level_x;                  // Current level position (scroll position)
} GameState;

GameState game;

// Level buffer - allocated dynamically based on level size
uint8_t *level_buffer = NULL;
int level_width = 0;

// Flag to control the game loop
volatile int keep_running = 1;

static int      map_origin   = 0;  // which device column (0–31) is the leftmost tile
static uint8_t  pixel_offset = 0;  // 0–31 pixel scroll inside a tile

// Thread for game logic
pthread_t game_thread;
pthread_mutex_t game_mutex = PTHREAD_MUTEX_INITIALIZER;

static int load_gameover(void);
static void show_gameover(int fd);


/**
 * Signal handler for clean termination
 */
void handle_signal(int sig) {
    keep_running = 0;
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

// A 20×15 tile image that spells out “GAME OVER”
static uint8_t gameover_map[SCREEN_HEIGHT][SCREEN_WIDTH];

// Read the 20×15 tile indices from disk into gameover_map[][]
static int load_gameover(void) {
    FILE *f = fopen(GAMEOVER_BIN, "rb");
    if (!f) { perror("Opening gameover.bin"); return -1; }
    size_t want = SCREEN_HEIGHT * SCREEN_WIDTH;
    size_t got  = fread(gameover_map, 1, want, f);
    fclose(f);
    if (got != want) {
        fprintf(stderr, "Expected %zu bytes in %s, got %zu\n", want, GAMEOVER_BIN, got);
        return -1;
    }
    return 0;
}

static void show_gameover(int fd) {
    geo_dash_arg_t arg;
    pthread_mutex_lock(&game_mutex);
    for (int row = 0; row < SCREEN_HEIGHT; row++) {
        for (int col = 0; col < SCREEN_WIDTH; col++) {
            arg.tilemap_row = row;
            arg.tilemap_col = col;
            arg.tile_value  = gameover_map[row][col];
            if (ioctl(fd, WRITE_TILE, &arg) < 0) {
                perror("WRITE_TILE gameover");
                pthread_mutex_unlock(&game_mutex);
                return;
            }
        }
    }
    pthread_mutex_unlock(&game_mutex);
	sleep(5);
}


void initial_fill(int fd) {
    map_origin = 0;
    for (int col = 0; col < 32; col++) {
        for (int row = 0; row < SCREEN_HEIGHT; row++) {
            geo_dash_arg_t arg = {
                .tilemap_row = row,
                .tilemap_col = col,
                .tile_value   = get_level_tile(row, game.level_x + col)
            };
            if (ioctl(fd, WRITE_TILE, &arg) < 0) {
                perror("Error writing tile");
                pthread_mutex_unlock(&game_mutex);
                return;
            }
        }
    }
    
    pthread_mutex_unlock(&game_mutex);
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
 * Check if a tile is solid (obstacle/ground)
 */
bool is_solid_tile(uint8_t tile) {
    // Tiles 1 (ground), 2 (platform), and 3 (obstacle) are considered solid
    return (tile == 1 || tile == 2 || tile == 3);
}

/**
 * Check if player is colliding with an obstacle
 */
bool check_collision(int player_screen_x, int player_screen_y) {
    // Get the absolute level position
    int level_x_pos = game.level_x + player_screen_x;
    
    // Check the tile at the player's position
    uint8_t tile = get_level_tile(player_screen_y, level_x_pos);
	printf("player_screen_y: %d, level_x_pos %d, tile is: %d\n", player_screen_y, level_x_pos, tile);
    
    // Obstacle is tile type 3 (red)
    return (tile == 1 || tile == 2);
}

/**
 * Update the visible tilemap on the device
 * @param fd The device file descriptor
 */
void update_screen(int fd) {
    geo_dash_arg_t arg;
    
    pthread_mutex_lock(&game_mutex);
    
    // Write each visible tile to the device
    for (int row = 0; row < SCREEN_HEIGHT; row++) {
        for (int col = 0; col < SCREEN_WIDTH; col++) {
            // Get the level column
            int level_col = game.level_x + col;
            
            // Get the tile from our level buffer
            uint8_t tile = get_level_tile(row, level_col);
            
            // Write the tile to the device
            arg.tilemap_row = row;
            arg.tilemap_col = col;
            arg.tile_value = tile;
            
            if (ioctl(fd, WRITE_TILE, &arg) < 0) {
                perror("Error writing tile");
                pthread_mutex_unlock(&game_mutex);
                return;
            }
        }
    }
    
    pthread_mutex_unlock(&game_mutex);
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
        0x00FFFF00,  // Color 7: Yellow
        0x00FF00FF   // Color 8: Purple (player)
    };
    
    // Write the palette colors to the device
    for (int i = 0; i < 9; i++) {
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
    int tile_count = 0;
    
    // Read tileset from file
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Error opening tileset file");
        return -1;
    }
    
    // Continue reading tiles until we reach EOF
    while (!feof(file)) {
        // Initialize the current tile with zeros
        for (int row = 0; row < 32; row++) {
            for (int col = 0; col < 32; col++) {
                arg.tileset[row][col] = 0;
            }
        }
        
        // Read a 32x32 tile from the file
        for (int row = 0; row < 32; row++) {
            for (int col = 0; col < 32; col++) {
                uint8_t byte;
                if (fread(&byte, 1, 1, file) != 1) {
                    if (feof(file)) {
                        // End of file reached
                        goto end_of_file;
                    } else {
                        perror("Error reading from file");
                        fclose(file);
                        return -2;
                    }
                }
                arg.tileset[row][col] = byte;
            }
        }
        
        // Write this tile to the device
        arg.tile_no = tile_count;
        if (ioctl(fd, WRITE_TILESET, &arg) < 0) {
            perror("Error writing tileset");
            fclose(file);
            return -3;
        }
        
        tile_count++;
    }
    
end_of_file:
    fclose(file);
    
    // Create a player sprite tile (if we have enough tile slots)
    // We'll use tile index 8 for our player
    if (tile_count <= 8) {
        // Initialize tile with zeros
        for (int row = 0; row < 32; row++) {
            for (int col = 0; col < 32; col++) {
                arg.tileset[row][col] = 0;
            }
        }
        
        // Create a simple player sprite - a colored square with eyes
        for (int row = 8; row < 24; row++) {
            for (int col = 8; col < 24; col++) {
                if (row == 8 || row == 23 || col == 8 || col == 23) {
                    // Border
                    arg.tileset[row][col] = 8;  // Purple outline
                } else if ((row == 12 && (col == 12 || col == 19)) || 
                          (row == 13 && (col == 12 || col == 19))) {
                    // Eyes
                    arg.tileset[row][col] = 0;  // Black eyes
                } else {
                    // Body fill
                    arg.tileset[row][col] = 8;  // Purple fill
                }
            }
        }
        
        // Write the player tile to the device
        arg.tile_no = PLAYER_TILE;
        if (ioctl(fd, WRITE_TILESET, &arg) < 0) {
            perror("Error writing player tile");
            return -4;
        }
        
        printf("Player sprite created as tile %d\n", PLAYER_TILE);
    } else {
        printf("Warning: Not enough tile slots for player sprite\n");
    }
    
    printf("Tileset successfully loaded and written to device (%d tiles)\n", tile_count);
    return 0;
}

/**
 * Initialize the game state
 */
void initialize_game() {
    game.player_x = PLAYER_START_X;
    game.player_y = REG_GROUND;
    game.player_vy = 0;
    game.is_jumping = false;
    game.is_dead = false;
    game.level_x = 0;
}

/**
 * Update player physics and game state
 */
void update_game_state(int fd) {
    pthread_mutex_lock(&game_mutex);
    // Get controller state
    ControllerState controller = controller_get_state();
    
    // Handle jump input
    if (controller.buttonAPressed && !game.is_jumping && game.player_y >= REG_GROUND) {
        game.player_vy = JUMP_VELOCITY;
        game.is_jumping = true;
    }
    
    // Update player vertical position with gravity
    game.player_vy += GRAVITY;
    
    // Clamp fall speed
    if (game.player_vy > MAX_FALL_SPEED) {
        game.player_vy = MAX_FALL_SPEED;
    }
      
    // Update player Y position

    // assume player_y and player_vy are uint32_t
    game.player_y += game.player_vy;

	if (game.player_y >= REG_GROUND) {
		game.player_y = REG_GROUND;
		game.player_vy = 0;
		game.is_jumping = false;
	}

	// Update hardware sprite position via ioctl
	geo_dash_arg_t arg;
	arg.player_y = (uint8_t)game.player_y;
	// printf("Writing player y pos to pos %d\n", arg.player_y);
	if (ioctl(fd, WRITE_PLAYER_Y_POS, &arg) < 0) {
		perror("Failed to write player Y position");
	}
    
    // Check for ground collision
    if (game.player_y >= REG_GROUND) {
        game.player_y = REG_GROUND;
        game.player_vy = 0;
        game.is_jumping = false;
    }

	int tile_x = (int)game.player_x;  
	int tile_y = (int)((game.player_y+113) / TILE_HEIGHT);
    
    // Check for obstacle collision
    if (check_collision(tile_x, tile_y)) {
		pthread_mutex_unlock(&game_mutex);
        game.is_dead = true;
		keep_running = 0;
        printf("\nGame Over! Collided with an obstacle.\n");
		show_gameover(fd);
		return;
    }

	/*
	// THEN: look one tile below our feet for ground (tile == 1)
    uint8_t below = get_level_tile(tile_y + 1, game.level_x + tile_x);
    if (below == 1) {
        // snap to the *top* of that block
        float ground_px = (tile_y + 1) * TILE_HEIGHT - TILE_HEIGHT - 113;
        game.player_y   = ground_px;
        game.player_vy  = 0;
        game.is_jumping = false;
        // push to hardware
        geo_dash_arg_t arg = { .player_y = (uint8_t)game.player_y };
        ioctl(fd, WRITE_PLAYER_Y_POS, &arg);
        pthread_mutex_unlock(&game_mutex);
        return;
    }
	*/
    
    pthread_mutex_unlock(&game_mutex);
}

/**
 * Game loop thread function
 */
void* game_loop(void* arg) {
    int fd = *((int*)arg);
    struct timespec sleep_time;
    
    // Set up timing
    int fps = 60;  // Target frames per second
    int frame_time_ms = 1000 / fps;
    
    // Sleep time structure for consistent frame rate
    sleep_time.tv_sec = 0;
    sleep_time.tv_nsec = frame_time_ms * 1000000; // Convert to nanoseconds
    printf("player y pos before running game loop: %d\n", game.player_y); 
    // Main game loop
    while (keep_running && game.level_x < level_width - SCREEN_WIDTH) {
        // Skip processing if game is over
        if (!game.is_dead) {
            // Update game state (player physics, etc.)
            update_game_state(fd);
            
            // Advance level position every few frames for scrolling
            static int scroll_counter = 0;
            if (++scroll_counter >= 15) {  // Scroll every 15 frames (4 times per second at 60fps)
                scroll_counter = 0;
                
                pthread_mutex_lock(&game_mutex);
                game.level_x++;
                pthread_mutex_unlock(&game_mutex);
                
                // Display some debug info
                printf("Level position: %d/%d | Player: (%.1f, %.1f) | %s\r", 
                       game.level_x, level_width - SCREEN_WIDTH, 
                       game.player_x, game.player_y,
                       game.is_jumping ? "Jumping" : "Grounded");
                fflush(stdout);
            }
        }


		// 2) smooth pixel scroll
        pixel_offset = (pixel_offset + 1) & 0x1F;
        geo_dash_arg_t a = { .scroll_offset = pixel_offset };
        if (ioctl(fd, WRITE_SCROLL_OFFSET, &a) < 0) {
            perror("WRITE_SCROLL_OFFSET");
        }

        // 3) if we just wrapped a full tile (32px), reload one new column
        if (pixel_offset == 0) {
            pthread_mutex_lock(&game_mutex);
            game.level_x++;
            int dead_col  = map_origin;
            int new_tilec = game.level_x + 31;
            for (int r = 0; r < SCREEN_HEIGHT; r++) {
                geo_dash_arg_t x = {
                    .tilemap_row = r,
                    .tilemap_col = dead_col,
                    .tile_value   = get_level_tile(r, new_tilec)
                };
                if (ioctl(fd, WRITE_TILE, &x) < 0)
                    perror("WRITE_TILE (repaint)");
            }
            map_origin = (map_origin + 1) & 31;
            pthread_mutex_unlock(&game_mutex);
        }
        
        // Update screen
        update_screen(fd);
        
        // Sleep to maintain frame rate
        nanosleep(&sleep_time, NULL);
    }
    
    printf("\nGame complete!\n");
    return NULL;
}

int main(int argc, char *argv[]) {
    int fd;
    
    // Check for required arguments
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <tileset_filename> <level_filename> <level_width>\n", argv[0]);
        fprintf(stderr, "  tileset_filename: Binary file containing tileset data\n");
        fprintf(stderr, "  level_filename: Binary file containing level data in row-major format\n");
        fprintf(stderr, "  level_width: Width of the level in tiles\n");
        return -1;
    }
    
    // Parse the level width
    level_width = atoi(argv[3]);
    if (level_width <= 0) {
        fprintf(stderr, "Error: Level width must be a positive integer\n");
        return -1;
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
    
	// Load game over
	if (load_gameover() < 0) {
		fprintf(stderr, "Failed to load %s\n", GAMEOVER_BIN);
		return 1;
	}
	
    // Initialize the controller
    controller_init();
    printf("player y pos at beginning: %d\n", game.player_y);    
    // Initialize game state
    initialize_game();
    
    printf("Starting Geometry Dash. Press Ctrl+C to exit.\n");
    printf("Level width: %d tiles\n", level_width);
    printf("Controls: Press A button to jump\n");
    printf("player y pos after initialization: %d\n", game.player_y); 

    // initial game state
    game.player_x = PLAYER_START_X;
    game.player_y = PLAYER_START_Y;
    game.player_vy = 0;
    game.is_jumping = false;
    game.is_dead    = false;
    game.level_x    = 0;

    // prime the tilemap ring
    initial_fill(fd);

	// Start game thread
    if (pthread_create(&game_thread, NULL, game_loop, &fd) != 0) {
        perror("Failed to create game thread");
        cleanup_level();
        close(fd);
        return -1;
    }
    
    // Wait for game thread to finish
    pthread_join(game_thread, NULL);
    
    // Clean up
    cleanup_level();
    close(fd);
    return 0;
}

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
#include <stdbool.h> 
#include "geo_dash.h"
#include "../controller/usbjoypad.h"

#define SCREEN_WIDTH  20
#define SCREEN_HEIGHT 15

// Player sprite constants
#define PLAYER_TILE     8    // Tile index for player sprite
#define PLAYER_START_X  5    // Starting X position (tile coordinate)
#define PLAYER_START_Y  12   // Starting Y position (tile coordinate)
#define GRAVITY         0.6f // Gravity force
#define JUMP_VELOCITY  -2.5f // Initial jump velocity (negative = up)
#define MAX_FALL_SPEED  3.0f // Max downward speed
#define GROUND_Y       12    // Ground Y (tile)

/**
 * GameState holds tile‐based scroll (level_x) and player physics.
 */
typedef struct {
    float player_x, player_y, player_vy;
    bool  is_jumping, is_dead;
    int   level_x;         // leftmost visible tile index
} GameState;

static GameState game;

// Level buffer
static uint8_t *level_buffer = NULL;
static int      level_width  = 0;

// Globals for our ring buffer
static int      map_origin   = 0;  // which device column (0–31) is the leftmost tile
static uint8_t  pixel_offset = 0;  // 0–31 pixel scroll inside a tile

// Control flags
static volatile int keep_running = 1;
static pthread_mutex_t game_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t       game_thread;

/**
 * Signal handler to stop the game loop cleanly.
 */
void handle_signal(int sig) {
    (void)sig;
    keep_running = 0;
}

/**
 * Load a level from disk into row-major buffer.
 */
int load_level(const char *filename, int width) {
    FILE *f = fopen(filename, "rb");
    if (!f) { perror("Error opening level file"); return -1; }
    level_width = width;
    level_buffer = malloc(SCREEN_HEIGHT * level_width);
    if (!level_buffer) {
        perror("Alloc level_buffer");
        fclose(f);
        return -1;
    }
    size_t bytes = fread(level_buffer, 1,
                         SCREEN_HEIGHT * level_width, f);
    printf("Read %zu bytes of level data\n", bytes);
    fclose(f);
    return 0;
}

/**
 * Get tile at (row, col) in the loaded level.
 */
static inline uint8_t get_level_tile(int row, int col) {
    if (row<0 || row>=SCREEN_HEIGHT || col<0 || col>=level_width)
        return 0;
    return level_buffer[row * level_width + col];
}

/**
 * Write the 32×15 initial window into the device as a ring buffer.
 */
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
                perror("WRITE_TILE (initial_fill)");
            }
        }
    }
}

/**
 * Initialize palette colors on the device.
 */
void initialize_palette(int fd) {
    static const uint32_t colors[9] = {
        0x00000000,  // 0: transparent
        0x00663300,  // 1: brown ground
        0x00888888,  // 2: gray platform
        0x00FF0000,  // 3: red obstacle
        0x00FFFFFF,  // 4: white cloud
        0x0000FF00,  // 5: green
        0x000000FF,  // 6: blue
        0x00FFFF00,  // 7: yellow
        0x00FF00FF   // 8: purple player
    };
    geo_dash_arg_t arg;
    for (int i = 0; i < 9; i++) {
        arg.color_index = i;
        arg.rgb         = colors[i];
        if (ioctl(fd, WRITE_PALETTE, &arg) < 0) {
            perror("WRITE_PALETTE");
        }
    }
}

/**
 * Load tileset (32×32 tiles) and create player sprite.
 */
int load_tileset(int fd, const char *filename) {
    geo_dash_arg_t arg;
    int tile_count = 0;
    FILE *f = fopen(filename, "rb");
    if (!f) { perror("open tileset"); return -1; }

    // Read raw tiles
    while (1) {
        if (feof(f)) break;
        for (int r = 0; r < 32; r++)
            for (int c = 0; c < 32; c++) {
                int b = fgetc(f);
                if (b == EOF) goto tiles_loaded;
                arg.tileset[r][c] = (uint8_t)b;
            }
        arg.tile_no = tile_count;
        if (ioctl(fd, WRITE_TILESET, &arg) < 0) {
            perror("WRITE_TILESET");
            fclose(f);
            return -1;
        }
        tile_count++;
    }
tiles_loaded:
    fclose(f);

    // Build purple square player if there's room
    if (tile_count <= PLAYER_TILE) {
        for (int r = 0; r < 32; r++)
            for (int c = 0; c < 32; c++)
                arg.tileset[r][c] = 0;
        for (int r = 8; r < 24; r++) {
            for (int c = 8; c < 24; c++) {
                if (r==8||r==23||c==8||c==23)
                    arg.tileset[r][c] = 8;
                else if ((r==12&&(c==12||c==19))||
                         (r==13&&(c==12||c==19)))
                    arg.tileset[r][c] = 0;
                else
                    arg.tileset[r][c] = 8;
            }
        }
        arg.tile_no = PLAYER_TILE;
        if (ioctl(fd, WRITE_TILESET, &arg) < 0) {
            perror("WRITE_TILESET player");
            return -1;
        }
        printf("Player sprite = tile %d\n", PLAYER_TILE);
    }
    printf("Loaded %d tileset tiles\n", tile_count);
    return 0;
}

/**
 * Update physics, sprite Y position, and check for death.
 */
void update_game_state(int fd) {
    pthread_mutex_lock(&game_mutex);
    ControllerState ctl = controller_get_state();
    if (ctl.buttonAPressed && !game.is_jumping && game.player_y >= GROUND_Y) {
        game.player_vy  = JUMP_VELOCITY;
        game.is_jumping = true;
    }
    // gravity
    game.player_vy += GRAVITY;
    if (game.player_vy > MAX_FALL_SPEED)
        game.player_vy = MAX_FALL_SPEED;
    game.player_y += game.player_vy;

    // clamp to ground
    if (game.player_y >= GROUND_Y) {
        game.player_y  = GROUND_Y;
        game.player_vy = 0;
        game.is_jumping = false;
    }

    // sprite Y ioctl
    geo_dash_arg_t arg = { .player_y = (uint16_t)game.player_y };
    if (ioctl(fd, WRITE_PLAYER_Y_POS, &arg) < 0)
        perror("WRITE_PLAYER_Y_POS");

    // collision
    int tile_under = get_level_tile((int)game.player_y,
                                    game.level_x + (int)game.player_x);
    if (tile_under == 3) {
        game.is_dead = true;
        printf("\nGame Over! Hit obstacle.\n");
    }
    pthread_mutex_unlock(&game_mutex);
}

/**
 * Thread: main game/render loop at ~60Hz.
 */
void *game_loop(void *argp) {
    int fd = *((int*)argp);
    struct timespec sleep_time = { .tv_sec = 0,
                                   .tv_nsec = (1000/60)*1000000 };

    while (keep_running && game.level_x < level_width - SCREEN_WIDTH) {
        // 1) physics & input
        if (!game.is_dead) {
            update_game_state(fd);
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

        // 4) frame pacing
        nanosleep(&sleep_time, NULL);
    }

    printf("\nLevel complete or exit!\n");
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr,"Usage: %s <tileset> <level> <level_width>\n",argv[0]);
        return 1;
    }
    signal(SIGINT,  handle_signal);
    signal(SIGTERM, handle_signal);

    int fd = open("/dev/geo_dash", O_RDWR);
    if (fd < 0) { perror("open /dev/geo_dash"); return 1; }

    if (load_tileset(fd, argv[1])  < 0) return 1;
    initialize_palette(fd);

    int w = atoi(argv[3]);
    if (w <= 0) { fprintf(stderr,"Bad level_width\n"); return 1; }
    if (load_level(argv[2], w)    < 0) return 1;

    // initial game state
    game.player_x = PLAYER_START_X;
    game.player_y = PLAYER_START_Y;
    game.player_vy = 0;
    game.is_jumping = false;
    game.is_dead    = false;
    game.level_x    = 0;

    // prime the tilemap ring
    initial_fill(fd);

    // start the loop
    if (pthread_create(&game_thread, NULL, game_loop, &fd) != 0) {
        perror("pthread_create");
        return 1;
    }
    pthread_join(game_thread, NULL);

    free(level_buffer);
    close(fd);
    return 0;
}
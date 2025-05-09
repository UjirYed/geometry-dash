#ifndef _GEO_DASH_H
#define _GEO_DASH_H

#ifndef __KERNEL__
#include <stdint.h>
#endif

#include <linux/ioctl.h>

// Game settings and constants
#define MAX_LEVEL_LENGTH 1024  // Maximum level length in blocks
#define MAX_OBSTACLES 16       // Maximum number of different obstacle types

// Obstacle type definitions
#define OBS_NONE 0             // Empty space
#define OBS_SPIKE 1            // Spike (instant death)
#define OBS_BLOCK 2            // Square block (collision)
#define OBS_PLATFORM 3         // Platform (can land on)
#define OBS_JUMP_PAD 4         // Jump pad (extra boost)
#define OBS_GRAVITY_PORTAL 5   // Gravity portal (flip gravity)

// Player state flags
#define PLAYER_NORMAL 0x00     // Default state
#define PLAYER_JUMPING 0x01    // Player is jumping
#define PLAYER_DEAD 0x02       // Player is dead
#define PLAYER_INVERTED 0x04   // Gravity is inverted

// Game state flags
#define GAME_LOADING 0x01      // Game is loading
#define GAME_READY 0x02        // Game is ready to start
#define GAME_PLAYING 0x04      // Game is in progress
#define GAME_OVER 0x08         // Game is over

// Structure for communicating with the device driver
typedef struct {
    uint16_t x_shift;          // Pixel offset for scrolling
    uint16_t player_y;         // Player Y position
    uint8_t  bg_r;             // Background color (R)
    uint8_t  bg_g;             // Background color (G)
    uint8_t  bg_b;             // Background color (B)
    uint8_t  map_block;        // Current map block
    uint8_t  flags;            // Game flags
    uint8_t  output_flags;     // Output status flags
    uint16_t audio;            // Audio sample
} geo_dash_arg_t;

// IOCTL commands
#define GEO_DASH_MAGIC 'q'

#define WRITE_X_SHIFT          _IOW(GEO_DASH_MAGIC, 0, geo_dash_arg_t *)
#define WRITE_PLAYER_Y_POS     _IOW(GEO_DASH_MAGIC, 1, geo_dash_arg_t *)
#define WRITE_BACKGROUND_R     _IOW(GEO_DASH_MAGIC, 2, geo_dash_arg_t *)
#define WRITE_BACKGROUND_G     _IOW(GEO_DASH_MAGIC, 3, geo_dash_arg_t *)
#define WRITE_BACKGROUND_B     _IOW(GEO_DASH_MAGIC, 4, geo_dash_arg_t *)
#define WRITE_MAP_BLOCK        _IOW(GEO_DASH_MAGIC, 5, geo_dash_arg_t *)
#define WRITE_FLAGS            _IOW(GEO_DASH_MAGIC, 6, geo_dash_arg_t *)
#define WRITE_OUTPUT_FLAGS     _IOW(GEO_DASH_MAGIC, 7, geo_dash_arg_t *)
#define WRITE_AUDIO_FIFO       _IOW(GEO_DASH_MAGIC, 9, geo_dash_arg_t *)
#define READ_AUDIO_FILL_LEVEL  _IOR(GEO_DASH_MAGIC, 10, uint32_t *)
#define READ_AUDIO_STATUS      _IOR(GEO_DASH_MAGIC, 11, uint32_t *)

#endif

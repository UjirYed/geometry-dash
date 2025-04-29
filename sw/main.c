#include <stdio.h>
#include <stdint.h>
// Main component:
// Will perform the following:
// it needs to set up taking input from the user button
// also needs access to the level_map in the hardware.


// the square moves at a constant rate player_speed.
// the square can also jump if button_pressed is true and the square is on the "ground"

typedef struct {
    int x_pos;  // pixels
    int y_pos;  // pixels
    int x_vel;  // pixels/frame
    int y_vel;  // pixels/frame
} Player;

Player player;
int button_pressed;
int x_shift;

#define LOADING 2
#define READY 4
#define PLAYING 6
int READY, SET = 0;

int midi_loaded = 0;
int audio_initialized = 0;

int current_state;

#define GROUND 10
#define PLAYER_SPEED 2
#define JUMP_VELOCITY 15
#define GRAVITY 1
#define BLOCK_SIZE 8
#define SCREEN_COLS 80
#define DISPLAY_HEIGHT 6
#define DISPLAY_WIDTH 128

uint16_t level_buf[1024][64]
uint16_t disp_buf[128][64];




int loadMapandMusic();
int runGamePhysics();
void updateDisplay();
int userPressedStartButton();
void startAudioPlayback();
void copyNextColumn();


int main() {

    while (1) {
        switch (current_state) {
            case LOADING:
                if (loadMapandMusic())
                    current_state = READY;
                    
                break;
            case READY:
                if (userPressedStartButton()) {
                    current_state = PLAYING;
                    startAudioPlayback();
                }
                break;
            case PLAYING:
                runGamePhysics();
                updateDisplay();
                break;


        }
    }
}

int loadMapAndMusic() {
    /* This function needs to copy fill display_buf from level_buf, and then write it to hardware. */
    return 1


}

int runGamePhysics() {
    if (button_pressed && player.y_pos == GROUND) {
        player.y_vel = JUMP_VELOCITY;
    }

    // player gravity
    if (player.y_pos < GROUND || player.y_vel > 0) {
        player.y_pos -= player.y_vel;
        player.y_vel -= GRAVITY;
        if (player.y_pos > GROUND) {
            player.y_pos = GROUND;
            player.y_vel = 0;
        }
    }

    // player speed
    player.x_pos += player.x_vel;
    x_shift += player.x_vel;

    if (x_shift >= BLOCK_SIZE) {
        x_shift -= BLOCK_SIZE;
        copyNextColumn();
    }

    return 1;
}

void copyNextColumn() {
    /* Copies the next column in the level to the display buf */
    int column_in_level = (x_shift / BLOCK_SIZE) + SCREEN_COLS;
    int column_in_display = (x_shift / BLOCK_SIZE) % DISPLAY_WIDTH;

    for (int row = 0; row < DISPLAY_HEIGHT; row++) {
        display_buf[row][column_in_display] = level_buf[row][column_in_level]
    }
}




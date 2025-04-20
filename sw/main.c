#include <stdio.h>

// Main component:
// Will perform the following:
// it needs to set up taking input from the user button
// also needs access to the level_map in the hardware.
//

#define LOADING 2
#define READY 4
#define 
int READY, SET = 0;

int current_state;

int main() {

    while (1) {
        switch (current_state) {
            case LOADING:
                loadMapAndMusic();
                break;
            case READY:


        }
    }
}

int loadMapAndMusic() {
    return 1;
}

int runGamePhysics() {
    return 1;
}



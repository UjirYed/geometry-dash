#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "usbjoypad.h"
#include <pthread.h>
int main() {
    controller_init();

    while (true) {
        ControllerState state = controller_get_state();

        printf("Left: %s | Right: %s | A: %s | Start: %s\n",
               state.leftArrowPressed ? "Pressed" : "Released",
               state.rightArrowPressed ? "Pressed" : "Released",
               state.buttonAPressed ? "Pressed" : "Released",
               state.startPressed ? "Pressed" : "Released");
        
        usleep(10000); // 10ms delay instead of sleep(0.001) which doesn't work as intended
    }

    return 0;
}

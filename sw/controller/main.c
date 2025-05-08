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

        //printf("%s %s %s %s\n");
        
        sleep(0.001);
    }

    return 0;
}

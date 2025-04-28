#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#include "geo_dash.h"

geo_dash_arg_t vla;

void set_player_xpos(unsigned short value){
    vla.player_pos_x = value;

    if (ioctl(geo_dash_fd, WRITE_PLAYER_XPOSITION , &vla)) {
      perror("ioctl(WRITE_DOG_POSITION) failed");
    }

}

int main() {

    set_player_xpos(24);
}

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <time.h>
#include "geo_dash.h"


int main() {
    geo_dash_arg_t arg;
    int fd = open("/dev/geo_dash", O_RDWR);
    if (fd < 0) {
        perror("Error opening device");
        return -1;
    }
    
    arg.tilemap_col = 0;
    arg.tilemap_row = 0;
    arg.tile_value = 1;

    ioctl(fd, WRITE_TILE, &arg);
    
    arg.rgb = 0xff0000;
    arg.i = 0;

    ioctl(fd, WRITE_PALETTE, &arg);
    
    
    // need to get a file, read it and put it into an array
    // set arg.tileset to this.
    arg.tileset = 

    ioctl(fd, WRITE_TILESET, &arg);
}

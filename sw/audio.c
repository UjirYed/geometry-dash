#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "geo_dash.h"

int main() {
    int fd = open("/dev/player_sprite_0", O_RDWR);
    if (fd < 0) {
        perror("Failed to open geo_dash");
        return 1;
    }

    FILE *audio = fopen("monody_stereo_48k.raw", "rb");
    if (!audio) {
        perror("Failed to open audio file");
        close(fd);
        return 1;
    }

    printf("opened audio and geo_dash device\n");
    geo_dash_arg_t arg = {0};
    uint16_t dummy;

    while (fread(&arg.audio, sizeof(uint16_t), 1, audio) == 1) {
		printf("Skipping right channel\n");
        // Skip right channel
        fread(&dummy, sizeof(uint16_t), 1, audio);

		printf("Attempting to ioctl\n");
        if (ioctl(fd, WRITE_AUDIO_FIFO, &arg) == -1) {
            perror("ioctl WRITE_AUDIO_FIFO failed");
            break;
        }

		sleep(1);
    }

    printf("cleaning up\n");
    fclose(audio);
    close(fd);
    return 0;
}

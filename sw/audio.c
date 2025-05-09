#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "geo_dash.h"

void print_fifo_status(uint32_t status) {
    printf("FIFO Status Register: 0x%08x\n", status);

    if (status & (1 << 0)) printf(" - FIFO is EMPTY\n");
    if (status & (1 << 1)) printf(" - FIFO is FULL\n");
    if (status & (1 << 2)) printf(" - FIFO is ALMOST EMPTY\n");
    if (status & (1 << 3)) printf(" - FIFO is ALMOST FULL\n");

    // Optional: Print raw bits if unsure of meaning
    for (int i = 4; i < 8; i++) {
        if (status & (1 << i))
            printf(" - Unknown bit %d set\n", i);
    }
}

int main() {
	int fd = open("/dev/audio_fifo", O_RDWR);
    if (fd < 0) {
		perror("Failed to open audio_fifo");
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

	int i = 0;

    while (fread(&arg.audio, sizeof(uint16_t), 1, audio) == 1) {
		printf("Skipping right channel\n");
        // Skip right channel
        fread(&dummy, sizeof(uint16_t), 1, audio);

		printf("Attempting to ioctl\n");
        if (ioctl(fd, WRITE_AUDIO_FIFO, &arg) == -1) {
            perror("ioctl WRITE_AUDIO_FIFO failed");
            break;
        }

		if ((i++ % 50) == 0) {
			if (ioctl(fd, READ_AUDIO_STATUS, &status) == -1) {
				perror("ioctl READ_AUDIO_STATUS failed");
			} else {
				print_fifo_status(status);
			}
		}

		usleep(100);
    }

    printf("cleaning up\n");
    fclose(audio);
    close(fd);
    return 0;
}

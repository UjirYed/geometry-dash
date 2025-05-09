#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "geo_dash.h"

void print_fifo_status(uint32_t status) {
    printf("FIFO i_status: 0x%08x\n", status);

    if (status & (1 << 0)) printf(" - FULL: FIFO is full\n");
    if (status & (1 << 1)) printf(" - EMPTY: FIFO is empty\n");
    if (status & (1 << 2)) printf(" - ALMOSTFULL: Fill level >= almostfull threshold\n");
    if (status & (1 << 3)) printf(" - ALMOSTEMPTY: Fill level <= almostempty threshold\n");
    if (status & (1 << 4)) printf(" - OVERFLOW: Write occurred when FIFO was full\n");
    if (status & (1 << 5)) printf(" - UNDERFLOW: Read occurred when FIFO was empty\n");

    // Sanity check
    if ((status & 0x3F) == 0)
        printf(" - FIFO is somewhere between ALMOSTEMPTY and ALMOSTFULL, not full or empty\n");
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
		
		uint32_t status;
		if ((i++ % 50) == 0) {
			if (ioctl(fd, READ_AUDIO_STATUS, &status) == -1) {
				perror("ioctl READ_AUDIO_STATUS failed");
				close(fd);
				return 1;
			} else {
				print_fifo_status(status);
			}

			uint32_t fill_level;
			if (ioctl(fd, READ_AUDIO_FILL_LEVEL, &fill_level) == -1) {
				perror("ioctl READ_AUDIO_FILL_LEVEL failed");
				close(fd);
				return 1;
			}

			printf("FIFO fill level: %u\n", fill_level);
		}

		usleep(100);
    }

    printf("cleaning up\n");
    fclose(audio);
    close(fd);
    return 0;
}

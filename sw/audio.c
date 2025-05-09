#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#include "audio_fifo.h"


#define I2C_DEV "/dev/i2c-0"  // Might be /dev/i2c-1 on some boards
#define WM8731_ADDR 0x1A

// Utility to send a 9-bit register: 7 bits address + 9 bits data
int wm8731_write(int i2c_fd, uint8_t reg, uint16_t data) {
    uint8_t buf[2];
    buf[0] = (reg << 1) | ((data >> 8) & 0x01);  // reg address (7 bits) + D8
    buf[1] = data & 0xFF;                        // D7..D0
    return write(i2c_fd, buf, 2);
}

void init_wm8731() {
    int i2c_fd = open(I2C_DEV, O_RDWR);
    if (i2c_fd < 0) {
        perror("open i2c");
        return;
    }

    if (ioctl(i2c_fd, I2C_SLAVE, WM8731_ADDR) < 0) {
        perror("ioctl I2C_SLAVE");
        close(i2c_fd);
        return;
    }

    // Soft reset
    wm8731_write(i2c_fd, 0x0F, 0x000);

    // Left headphone out: volume = 0x79
    wm8731_write(i2c_fd, 0x02, 0x179);
    // Right headphone out: volume = 0x79
    wm8731_write(i2c_fd, 0x03, 0x179);

    // Analog audio path: DAC selected
    wm8731_write(i2c_fd, 0x04, 0x012);

    // Digital audio path: disable soft mute
    wm8731_write(i2c_fd, 0x05, 0x000);

    // Power down control: everything on
    wm8731_write(i2c_fd, 0x06, 0x000);

    // Digital audio interface format: I2S, 16-bit, MCLK slave (left justified)
    wm8731_write(i2c_fd, 0x07, 0x000);

    // Sampling control: normal mode, 48kHz
    wm8731_write(i2c_fd, 0x08, 0x000);

    // Activate digital interface
    wm8731_write(i2c_fd, 0x09, 0x001);

    close(i2c_fd);
}

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
	printf("Initializing Audio CODEC\n");
	init_wm8731();

	usleep(1000);

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

    printf("Opened audio_fifo device and audio file\n");
    audio_fifo_arg_t arg = {0};
    uint16_t dummy;
	

	//uint32_t fill_level;
	//if (ioctl(fd, READ_AUDIO_FILL_LEVEL, &fill_level) == -1) {
	//	perror("ioctl READ_AUDIO_FILL_LEVEL failed");
	//	close(fd);
	//	return 1;
	//}

	//printf("Initial FIFO fill level: %u\n", fill_level);

	int i = 0;
    while (fread(&arg.audio, sizeof(uint16_t), 1, audio) == 1) {
		// printf("Skipping right channel\n");
        // Skip right channel
        fread(&dummy, sizeof(uint16_t), 1, audio);  // skip right

		// Shift left sample into upper 16 bits, right = 0
		arg.audio = (uint32_t)arg.audio;

		if (ioctl(fd, WRITE_AUDIO_FIFO, &arg) == -1) {
			perror("ioctl WRITE_AUDIO_FIFO failed");
			break;
		}
	/*	
		uint32_t status;
		if ((i++ % 1000) == 0) {
			
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
*/
		usleep(100);
    }

    printf("cleaning up\n");
    fclose(audio);
    close(fd);
    return 0;
}

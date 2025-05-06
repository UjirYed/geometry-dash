#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>

#define HACTIVE 640
#define VACTIVE 480

typedef struct { uint8_t red, green, blue; uint8_t _padto32; } rgb_t; // 24-bit color

void *mapfile(const char *filename, size_t length)
{
  int fd = open(filename, O_RDONLY);
  if (fd == -1)
    fprintf(stderr, "Error opening \"%s\": ", filename), perror(NULL), exit(1);
  void *p = mmap(NULL, length, PROT_READ, MAP_SHARED, fd, 0);
  if (p == MAP_FAILED)
    fprintf(stderr, "Error mapping \"%s\": ", filename), perror(NULL), exit(1);
  close(fd);
  return p;
}

int main(int argc, const char *argv[])
{
  if (argc != 4)
    fprintf(stderr, "Usage: tiles2ppm <tilemap> <tileset> <palette>\n"), exit(1);
  
  uint8_t *tilemap = (uint8_t *) mapfile(argv[1], 8192);
  uint8_t *tileset = (uint8_t *) mapfile(argv[2], 16384);
  rgb_t   *palette = (rgb_t *)   mapfile(argv[3], 16 * sizeof(rgb_t));

  printf("P3\n%d %d\n255\n", HACTIVE, VACTIVE); // Plain PPM header, 24 bpp

  uint16_t x, y;
  for (y = 0 ; y < VACTIVE ; y++)
    for (x = 0 ; x < HACTIVE ; x++) {               // The tile algorithm:
      uint8_t r     = y >> 3;                       // Row          0-59
      uint8_t c     = x >> 3;                       // Column       0-79
      uint8_t t     = tilemap[r << 7 | c];          // Tile number  0-255
      uint8_t i     = x & 0x7;                      // Tile local x 0-7
      uint8_t j     = y & 0x7;                      // Tile local y 0-7
      uint8_t color = tileset[t << 6 | j << 3 | i]; // Color        0-15
      rgb_t rgb     = palette[color];               // RGB color    24 bits
      printf("%d %d %d\n", rgb.red, rgb.green, rgb.blue);
    }
  
  return 0;
}

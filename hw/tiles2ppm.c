#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>

#define HACTIVE 640
#define VACTIVE 480
#define TILE_SIZE 32  // Changed from 8 to 32

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
  
  // Update memory sizes for 32x32 tiles
  // 640/32 = 20 columns, 480/32 = 15 rows = 300 tiles (rounded to 512)
  uint8_t *tilemap = (uint8_t *) mapfile(argv[1], 512);
  // 256 tiles × 32×32 × 4 bits = 131,072 bits = 16,384 bytes
  uint8_t *tileset = (uint8_t *) mapfile(argv[2], 16384);
  rgb_t   *palette = (rgb_t *)   mapfile(argv[3], 16 * sizeof(rgb_t));

  printf("P3\n%d %d\n255\n", HACTIVE, VACTIVE); // Plain PPM header, 24 bpp

  uint16_t x, y;
  for (y = 0 ; y < VACTIVE ; y++)
    for (x = 0 ; x < HACTIVE ; x++) {               // The tile algorithm:
      uint8_t r     = y >> 5;                       // Row          0-14 (changed from >> 3)
      uint8_t c     = x >> 5;                       // Column       0-19 (changed from >> 3)
      uint8_t t     = tilemap[r << 5 | c];          // Tile number  0-255 (changed from << 7)
      uint8_t i     = x & 0x1F;                     // Tile local x 0-31 (changed from 0x7)
      uint8_t j     = y & 0x1F;                     // Tile local y 0-31 (changed from 0x7)
      uint8_t color = tileset[t << 10 | j << 5 | i]; // Color        0-15 (changed from << 6 and << 3)
      rgb_t rgb     = palette[color];               // RGB color    24 bits
      printf("%d %d %d\n", rgb.red, rgb.green, rgb.blue);
    }
  
  return 0;
}

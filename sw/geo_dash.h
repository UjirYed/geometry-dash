#ifndef _GEO_DASH_H
#define _GEO_DASH_H

#ifndef __KERNEL__
#include <stdint.h>
#endif

#include <linux/ioctl.h>

typedef struct {
    uint16_t x_shift;
    uint16_t player_y;
    uint8_t  bg_r;
    uint8_t  bg_g;
    uint8_t  bg_b;
    uint8_t  map_block;
    uint8_t  flags;
    uint8_t  output_flags;
    uint16_t audio;
} geo_dash_arg_t;


#define GEO_DASH_MAGIC 'q'

#define WRITE_X_SHIFT _IOW(GEO_DASH_MAGIC, 0, geo_dash_arg_t *)

#define WRITE_AUDIO_FIFO _IOW(GEO_DASH_MAGIC, 9, geo_dash_arg_t *)


#endif

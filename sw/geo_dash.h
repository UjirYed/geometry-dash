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

#define WRITE_X_SHIFT 		 _IOW(GEO_DASH_MAGIC, 0, geo_dash_arg_t *)
#define WRITE_PLAYER_Y_POS   _IOW(GEO_DASH_MAGIC, 1, geo_dash_arg_t *)
#define WRITE_BACKGROUND_R   _IOW(GEO_DASH_MAGIC, 2, geo_dash_arg_t *)
#define WRITE_BACKGROUND_G   _IOW(GEO_DASH_MAGIC, 3, geo_dash_arg_t *)
#define WRITE_BACKGROUND_B   _IOW(GEO_DASH_MAGIC, 4, geo_dash_arg_t *)
#define WRITE_MAP_BLOCK      _IOW(GEO_DASH_MAGIC, 5, geo_dash_arg_t *)
#define WRITE_FLAGS          _IOW(GEO_DASH_MAGIC, 6, geo_dash_arg_t *)
#define WRITE_OUTPUT_FLAGS   _IOW(GEO_DASH_MAGIC, 7, geo_dash_arg_t *)
#define WRITE_AUDIO_FIFO 	 _IOW(GEO_DASH_MAGIC, 9, geo_dash_arg_t *)


#endif

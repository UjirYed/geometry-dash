#ifndef _GEO_DASH_H
#define _GEO_DASH_H

#ifndef __KERNEL__
#include <stdint.h>
#endif

#include <linux/ioctl.h>

typedef struct {
    uint16_t player_pos_x;

} geo_dash_arg_t;

#define GEO_DASH_MAGIC 'q'

#define WRITE_PLAYER_XPOSITION _IOW(GEO_DASH_MAGIC, 1, geo_dash_arg_t *)

#define READ_TEST_REG _IOR(CAT_INVADERS_MAGIC, 21, cat_invaders_arg_t *)


#endif

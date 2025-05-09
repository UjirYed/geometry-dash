#ifndef _AUDIO_FIFO_H
#define _AUDIO_FIFO_H

#ifndef __KERNEL__
#include <stdint.h>
#endif

#include <linux/ioctl.h>

typedef struct {
    uint32_t audio; // or any structure matching what audio_fifo_ioctl expects
} audio_fifo_arg_t;

#define AUDIO_FIFO_MAGIC 'r'
#define WRITE_AUDIO_FIFO       _IOW(AUDIO_FIFO_MAGIC, 1, audio_fifo_arg_t *)
#define READ_AUDIO_FILL_LEVEL  _IOR(AUDIO_FIFO_MAGIC, 2, uint32_t *)
#define READ_AUDIO_STATUS      _IOR(AUDIO_FIFO_MAGIC, 3, uint32_t *)

#endif

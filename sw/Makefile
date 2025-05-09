ifneq ($(KERNELRELEASE),)

# Kernel build context
obj-m := geo_dash.o audio_fifo.o

else

KERNEL_SOURCE := /usr/src/linux-headers-$(shell uname -r)
PWD := $(shell pwd)

default: module audio

module:
	$(MAKE) -C $(KERNEL_SOURCE) SUBDIRS=$(PWD) modules

audio: audio.c audio_fifo.h
	gcc -Wall -o audio audio.c

clean:
	$(MAKE) -C $(KERNEL_SOURCE) SUBDIRS=$(PWD) clean
	rm -f audio

TARFILES = Makefile geo_dash.h geo_dash.c audio.c audio_fifo.c audio_fifo.h
TARFILE = sw.tar.gz
.PHONY: tar
tar: $(TARFILE)

$(TARFILE): $(TARFILES)
	tar zcfC $(TARFILE) .. $(TARFILES:%=sw/%)

endif

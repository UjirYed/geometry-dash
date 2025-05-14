ifneq ($(KERNELRELEASE),)

# === Kernel build context ===
obj-m := geo_dash.o audio_fifo.o

else

# === User-space context ===

# --- General setup ---
C := gcc
CFLAGS := -Wall -Werror -g -O2 -std=c99 -D_GNU_SOURCE
LDFLAGS := -lm -lpthread -lusb-1.0

# Directories
CONTROLLER_DIR := controller
KERNEL_SOURCE := /usr/src/linux-headers-$(shell uname -r)
PWD := $(shell pwd)

# === User-Space Game Files ===
GAME_SRC := game_loop.c
CONTROLLER_SRC := $(CONTROLLER_DIR)/usbjoypad.c
INCLUDES := -I. -I$(CONTROLLER_DIR)
GAME_TARGET := geo_dash_game

# === Hello Program ===
HELLO_SRC := hello.c
HELLO_TARGET := hello

# === Tarball Packaging ===
TARFILES := Makefile geo_dash.h geo_dash.c hello.c controller/usbjoypad.c controller/usbjoypad.h
TARFILE := sw.tar.gz

# === Targets ===
default: module $(GAME_TARGET) $(HELLO_TARGET)

# Kernel module build
module:
	$(MAKE) -C $(KERNEL_SOURCE) SUBDIRS=$(PWD) modules

# Build user-space game
$(GAME_TARGET): $(GAME_SRC) $(CONTROLLER_SRC)
	$(C) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)

# Build hello test
$(HELLO_TARGET): $(HELLO_SRC)
	$(C) $(CFLAGS) -o $@ $<

# Clean everything
clean:
	$(MAKE) -C $(KERNEL_SOURCE) SUBDIRS=$(PWD) clean
	rm -f $(GAME_TARGET) $(HELLO_TARGET) *.o

# Archive source
.PHONY: tar
tar: $(TARFILE)

$(TARFILE): $(TARFILES)
	tar zcfC $(TARFILE) .. $(TARFILES:%=sw/%)

endif

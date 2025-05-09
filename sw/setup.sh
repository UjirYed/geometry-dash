#!/bin/bash

set -e  # Exit immediately if any command fails

echo "Rebuilding kernel modules..."
make -C /usr/src/linux-headers-$(uname -r) SUBDIRS=$(pwd) modules

echo "Removing old modules (if loaded)..."
rmmod geo_dash 2>/dev/null || echo "geo_dash was not loaded."
rmmod audio_fifo 2>/dev/null || echo "audio_fifo was not loaded."

echo "Inserting new modules..."
insmod audio_fifo.ko
insmod geo_dash.ko

echo "Building userspace program..."
make audio

echo "Setup complete."

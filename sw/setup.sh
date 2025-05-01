#!/bin/bash

set -e  # Exit if any command fails

echo "Rebuilding kernel module..."
make -C /usr/src/linux-headers-$(uname -r) SUBDIRS=$(pwd) modules

echo "Removing old module (if loaded)..."
rmmod geo_dash 2>/dev/null || echo "Module was not loaded."

echo "Inserting new module..."
insmod geo_dash.ko

echo "Building userspace program..."
make audio

echo "Setup complete."

# Optional: run the userspace app
# echo "Running hello..."
# ./hello

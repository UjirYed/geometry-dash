#!/bin/bash

# Simple script to compile and run the controller

cd "$(dirname "$0")"  # Change to script directory

# Check if the device file exists
if [ ! -c "/dev/player_sprite_0" ]; then
    echo "Warning: Device file /dev/player_sprite_0 not found."
    echo "This suggests the kernel module may not be loaded."
    echo "The controller will still compile but may not function correctly."
    echo ""
    echo "Press Enter to continue anyway, or Ctrl+C to cancel..."
    read
fi

# Compile the controller
echo "Compiling controller..."
make clean
make

# Run the controller
echo "Starting controller..."
sudo ./main 
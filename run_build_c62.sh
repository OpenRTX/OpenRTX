#!/bin/bash

# Call this file with "lisa zep exec ./run_build_c62.sh"

# Make call via lisa easier
rm -rf build; meson setup build; meson compile -C build openrtx_c62

# Do this at your own risk! This may brick your device or start a fire!
# lisa zep exec cskburn -s /dev/ttyUSB0 -C 6 -b 115200 0x000000 build/zephyr/zephyr.hex 
# Do this at your own risk! This may brick your device or start a fire!

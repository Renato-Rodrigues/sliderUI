#!/bin/sh
# toggle_mode.sh - toggle via file flag used by MinUI toggle app
FLAG="/mnt/SDCARD/App/sliderUI/.use_sliderui"

if [ -f "$FLAG" ]; then
    rm -f "$FLAG"
    echo "Switched to Full ROM Mode"
else
    touch "$FLAG"
    echo "Switched to sliderUI Mode"
fi

sync
# optional reboot - comment out if not desired
# reboot
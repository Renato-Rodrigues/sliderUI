#!/bin/sh
# toggle_sliderUI_autorun.sh - toggles autorun by renaming autorun script
AUTORUN_FILE="/mnt/SDCARD/.minui/autorun/launch_sliderUI.sh"

if [ -f "$AUTORUN_FILE" ]; then
    echo "Disabling SliderUI autorun..."
    mv "$AUTORUN_FILE" "${AUTORUN_FILE}.disabled"
    echo "Autorun disabled."
else
    if [ -f "${AUTORUN_FILE}.disabled" ]; then
        echo "Re-enabling SliderUI autorun..."
        mv "${AUTORUN_FILE}.disabled" "$AUTORUN_FILE"
        chmod +x "$AUTORUN_FILE"
        echo "Autorun enabled."
    else
        echo "No autorun launcher found. Run installer first."
    fi
fi
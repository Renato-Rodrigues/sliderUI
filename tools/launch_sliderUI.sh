#!/bin/sh
# launch_sliderUI.sh - autorun launcher
APP_PATH="/mnt/SDCARD/App/sliderUI/run_sliderUI.sh"
CFG_PATH="/mnt/SDCARD/Roms/sliderUI.cfg"

if [ ! -x "$APP_PATH" ]; then
    echo "[SliderUI] Launcher not found at $APP_PATH"
    exit 1
fi

echo "[SliderUI] Launching Kids Mode..."
$APP_PATH --config "$CFG_PATH"
RET=$?

# if exit code 42 is used to indicate Konami unlock
if [ "$RET" = "42" ]; then
    echo "[SliderUI] Konami code detected, starting MinUI..."
    /mnt/SDCARD/.minui/minui.sh &
else
    echo "[SliderUI] Exited with code $RET"
fi

exit 0
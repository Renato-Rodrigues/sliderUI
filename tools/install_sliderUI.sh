#!/bin/sh
# install_sliderUI.sh
APP_NAME="sliderUI"
APP_TITLE="Slider Mode"
APP_DESC="A kid-friendly game carousel UI with reflective box art."
APP_PATH="/mnt/SDCARD/App/${APP_NAME}"
BINARY_PATH="$(dirname "$0")/../build/sliderUI"
ICON_SOURCE="$(dirname "$0")/../assets/icons/app_icon.png"

echo "=== Installing ${APP_TITLE} into MinUI... ==="

if [ ! -d "${APP_PATH}" ]; then
    echo "Creating app directory: ${APP_PATH}"
    mkdir -p "${APP_PATH}"
fi

if [ -f "${BINARY_PATH}" ]; then
    cp "${BINARY_PATH}" "${APP_PATH}/${APP_NAME}"
    chmod +x "${APP_PATH}/${APP_NAME}"
    echo "Binary copied."
else
    echo "Error: compiled binary not found at ${BINARY_PATH}"
    exit 1
fi

if [ -f "${ICON_SOURCE}" ]; then
    cp "${ICON_SOURCE}" "${APP_PATH}/icon.png"
    echo "Icon installed."
fi

cat > "${APP_PATH}/metadata.txt" <<EOF
title=${APP_TITLE}
description=${APP_DESC}
exec=/mnt/SDCARD/App/${APP_NAME}/run_sliderUI.sh
EOF
echo "Metadata file created."

if [ ! -f "/mnt/SDCARD/Roms/sliderUI_games.txt" ]; then
    echo "Copying default game list..."
    cp "$(dirname "$0")/../data/sliderUI_games.txt" "/mnt/SDCARD/Roms/sliderUI_games.txt"
fi

if [ ! -f "/mnt/SDCARD/Roms/sliderUI.cfg" ]; then
    echo "Copying default config..."
    cp "$(dirname "$0")/../config/sliderUI.cfg" "/mnt/SDCARD/Roms/sliderUI.cfg"
fi

echo "=== ${APP_TITLE} installed successfully! ==="
exit 0
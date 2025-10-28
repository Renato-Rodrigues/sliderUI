#!/usr/bin/env bash
set -euo pipefail
SD=${1:-}
if [ -z "$SD" ]; then
  echo "Usage: $0 /path/to/sd_mount"
  exit 1
fi

WORKSPACE="$(pwd)/workspace/sliderUI"
BUILD="$WORKSPACE/build"
if [ ! -d "$BUILD" ]; then
  echo "Build directory not found: $BUILD"
  exit 2
fi

echo "[deploy] SD Card mount: $SD"
if [ ! -d "$SD" ]; then
  echo "SD mount path does not exist: $SD"
  exit 3
fi

mkdir -p "$SD/App/sliderUI_installer"
mkdir -p "$SD/App/sliderUI"
mkdir -p "$SD/Roms"

echo "[deploy] copying installer..."
cp -v "$BUILD/sliderUI_installer" "$SD/App/sliderUI_installer/sliderUI_installer"
chmod +x "$SD/App/sliderUI_installer/sliderUI_installer"
if [ -d "$WORKSPACE/assets" ]; then cp -r "$WORKSPACE/assets" "$SD/App/sliderUI_installer/" || true; fi
if [ -d "$WORKSPACE/data" ]; then cp -r "$WORKSPACE/data" "$SD/App/sliderUI_installer/" || true; fi
if [ -d "$WORKSPACE/config" ]; then cp -r "$WORKSPACE/config" "$SD/App/sliderUI_installer/" || true; fi
if [ -d "$WORKSPACE/tools" ]; then mkdir -p "$SD/App/sliderUI_installer/tools"; cp -r "$WORKSPACE/tools"/* "$SD/App/sliderUI_installer/tools/" || true; fi

cat > "$SD/App/sliderUI_installer/metadata.txt" <<EOF
title=SliderUI Installer
description=Install SliderUI from the MinUI menu (no terminal)
exec=/mnt/SDCARD/App/sliderUI_installer/sliderUI_installer
EOF

echo "[deploy] copying main app (optional)..."
cp -v "$BUILD/sliderUI" "$SD/App/sliderUI/sliderUI"
chmod +x "$SD/App/sliderUI/sliderUI"
cat > "$SD/App/sliderUI/metadata.txt" <<EOF
title=Slider Mode
description=Kid-friendly slider UI (direct run)
exec=/mnt/SDCARD/App/sliderUI/run_sliderUI.sh
EOF

# Copy bundle/lib if created
if [ -d "$WORKSPACE/deploy" ]; then
  echo "[deploy] copying deploy bundle..."
  cp -r "$WORKSPACE/deploy"/* "$SD/App/sliderUI/" || true
fi

cp -v "$BUILD/"*.sha256 "$SD/App/sliderUI_installer/" || true
sync
echo "[deploy] Done. You may now eject the SD card and insert into your Miyoo Mini Plus."
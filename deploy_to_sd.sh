#!/usr/bin/env bash
set -euo pipefail

SD="${1:-}"

if [ -z "$SD" ]; then
    echo "Usage: $0 /path/to/sd_mount"
    echo ""
    echo "Example: $0 /media/\$USER/MYSD"
    exit 1
fi

# Get project root
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD="$PROJECT_ROOT/build"

echo "=========================================="
echo "Deploying sliderUI to SD card"
echo "=========================================="
echo "SD Card mount: $SD"
echo "Project root: $PROJECT_ROOT"
echo ""

# Check build directory
if [ ! -d "$BUILD" ]; then
    echo "ERROR: Build directory not found: $BUILD"
    echo "Please run ./build_in_toolchain.sh first"
    exit 2
fi

# Check SD card mount
if [ ! -d "$SD" ]; then
    echo "ERROR: SD mount path does not exist: $SD"
    exit 3
fi

# Check if we have write permission
if [ ! -w "$SD" ]; then
    echo "ERROR: No write permission to $SD"
    echo "You may need to run with sudo or check mount permissions"
    exit 4
fi

# Create directories
echo "[deploy] Creating directories..."
mkdir -p "$SD/App/sliderUI_installer"
mkdir -p "$SD/App/sliderUI"

# Copy installer
echo "[deploy] Copying installer..."
if [ ! -f "$BUILD/sliderUI_installer" ]; then
    echo "ERROR: Installer binary not found: $BUILD/sliderUI_installer"
    exit 5
fi

cp "$BUILD/sliderUI_installer" "$SD/App/sliderUI_installer/sliderUI_installer"
chmod +x "$SD/App/sliderUI_installer/sliderUI_installer"

# Copy installer resources
if [ -d "$PROJECT_ROOT/assets" ]; then
    echo "[deploy] Copying assets..."
    cp -r "$PROJECT_ROOT/assets" "$SD/App/sliderUI_installer/"
fi

if [ -d "$PROJECT_ROOT/data" ]; then
    echo "[deploy] Copying data..."
    cp -r "$PROJECT_ROOT/data" "$SD/App/sliderUI_installer/"
fi

if [ -d "$PROJECT_ROOT/config" ]; then
    echo "[deploy] Copying config..."
    cp -r "$PROJECT_ROOT/config" "$SD/App/sliderUI_installer/"
fi

if [ -d "$PROJECT_ROOT/tools" ]; then
    echo "[deploy] Copying tools..."
    mkdir -p "$SD/App/sliderUI_installer/tools"
    cp "$PROJECT_ROOT/tools"/* "$SD/App/sliderUI_installer/tools/" 2>/dev/null || true
fi

# Create installer metadata
echo "[deploy] Creating installer metadata..."
cat > "$SD/App/sliderUI_installer/metadata.txt" <<'EOF'
title=SliderUI Installer
description=Install SliderUI from the MinUI menu (no terminal)
exec=/mnt/SDCARD/App/sliderUI_installer/sliderUI_installer
EOF

# Copy main app
echo "[deploy] Copying main app..."
if [ -f "$BUILD/sliderUI" ]; then
    cp "$BUILD/sliderUI" "$SD/App/sliderUI/sliderUI"
    chmod +x "$SD/App/sliderUI/sliderUI"
else
    echo "WARNING: Main binary not found: $BUILD/sliderUI"
fi

# Copy deploy bundle if exists
if [ -d "$PROJECT_ROOT/deploy" ]; then
    echo "[deploy] Copying bundled libraries..."
    cp -r "$PROJECT_ROOT/deploy"/* "$SD/App/sliderUI/" 2>/dev/null || true
fi

# Create app metadata
echo "[deploy] Creating app metadata..."
if [ -f "$PROJECT_ROOT/deploy/run_sliderUI.sh" ] && [ -f "$SD/App/sliderUI/run_sliderUI.sh" ]; then
    cat > "$SD/App/sliderUI/metadata.txt" <<'EOF'
title=Slider Mode
description=Kid-friendly slider UI
exec=/mnt/SDCARD/App/sliderUI/run_sliderUI.sh
EOF
else
    cat > "$SD/App/sliderUI/metadata.txt" <<'EOF'
title=Slider Mode
description=Kid-friendly slider UI
exec=/mnt/SDCARD/App/sliderUI/sliderUI
EOF
fi

# Copy checksums
if [ -f "$BUILD/sliderUI.sha256" ]; then
    cp "$BUILD"/*.sha256 "$SD/App/sliderUI_installer/" 2>/dev/null || true
fi

# Sync
sync

echo ""
echo "=========================================="
echo "Deploy SUCCESS!"
echo "=========================================="
echo "Files copied to:"
echo "  - $SD/App/sliderUI_installer/"
echo "  - $SD/App/sliderUI/"
echo ""
echo "You may now safely eject the SD card."
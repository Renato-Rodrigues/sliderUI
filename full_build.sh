#!/usr/bin/env bash
set -euo pipefail

echo "=========================================="
echo "Complete SliderUI Build Process"
echo "=========================================="

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_ROOT"

# Step 1: Download stb_image_write.h if missing
if [ ! -f "src/stb_image_write.h" ]; then
    echo "[1/4] Downloading stb_image_write.h..."
    curl -o src/stb_image_write.h https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h
    echo "✓ Downloaded stb_image_write.h"
else
    echo "[1/4] stb_image_write.h already exists"
fi

# Step 2: Build binaries
echo ""
echo "[2/4] Building sliderUI..."
./build_in_toolchain.sh

if [ $? -ne 0 ]; then
    echo "❌ Build failed!"
    exit 1
fi

# Step 3: Bundle libraries
echo ""
echo "[3/4] Bundling libraries..."
docker run --rm \
  -u "$(id -u):$(id -g)" \
  -v "$PROJECT_ROOT":/app \
  -w /app \
  union-miyoomini-toolchain:latest \
  /bin/bash -c "make bundle && make install-wrapper"

if [ $? -ne 0 ]; then
    echo "⚠ Bundle/wrapper creation failed (non-critical)"
fi

# Step 4: Verify SDL version
echo ""
echo "[4/4] Verifying SDL dependencies..."
echo "Expected: libSDL-1.2.so.0 (NOT libSDL2)"

docker run --rm \
  -v "$PROJECT_ROOT":/app \
  -w /app \
  union-miyoomini-toolchain:latest \
  /opt/miyoomini-toolchain/bin/arm-linux-gnueabihf-readelf -d build/sliderUI | grep "libSDL"

echo ""
echo "=========================================="
echo "✓ Build Complete!"
echo "=========================================="
echo ""
echo "Artifacts:"
ls -lh build/
echo ""
echo "Next steps:"
echo "1. Insert SD card"
echo "2. Run: ./deploy_to_sd.sh /media/\$USER/SDCARD_MOUNT"
echo "3. Safely eject and test on device"
#!/usr/bin/env bash
set -euo pipefail

# Get the absolute path to the project directory
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Use local toolchain image
IMAGE_NAME="union-miyoomini-toolchain:latest"

echo "=========================================="
echo "Building sliderUI"
echo "=========================================="
echo "Project root: $PROJECT_ROOT"
echo "Image: $IMAGE_NAME"
echo ""

# Check if image exists
if ! docker images | grep -q "union-miyoomini-toolchain"; then
    echo "ERROR: Docker image 'union-miyoomini-toolchain' not found!"
    echo "Please build it first with:"
    echo "  cd /path/to/union-miyoomini-toolchain"
    echo "  docker build -t union-miyoomini-toolchain:latest ."
    exit 1
fi

# Download stb_image_write.h if missing
if [ ! -f "$PROJECT_ROOT/src/stb_image_write.h" ]; then
    echo "[build] Downloading stb_image_write.h..."
    curl -o "$PROJECT_ROOT/src/stb_image_write.h" \
        https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h
    echo "[build] Downloaded stb_image_write.h"
fi

echo "[build] Running build in container..."

docker run --rm \
  -u "$(id -u):$(id -g)" \
  -v "$PROJECT_ROOT":/app \
  -w /app \
  "$IMAGE_NAME" \
  /bin/bash -c "set -e && echo 'Container working directory:' && pwd && make clean && make -j\$(nproc) all"

BUILD_EXIT=$?

if [ $BUILD_EXIT -eq 0 ]; then
    echo ""
    echo "=========================================="
    echo "Build SUCCESS!"
    echo "=========================================="
    echo "Artifacts:"
    ls -lh "$PROJECT_ROOT/build/"
    echo ""
    if [ -f "$PROJECT_ROOT/build/sliderUI.sha256" ]; then
        cat "$PROJECT_ROOT/build/sliderUI.sha256"
    fi
    if [ -f "$PROJECT_ROOT/build/sliderUI_installer.sha256" ]; then
        cat "$PROJECT_ROOT/build/sliderUI_installer.sha256"
    fi
else
    echo ""
    echo "=========================================="
    echo "Build FAILED!"
    echo "=========================================="
    exit $BUILD_EXIT
fi
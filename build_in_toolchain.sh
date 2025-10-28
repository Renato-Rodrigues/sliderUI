#!/usr/bin/env bash
set -euo pipefail
SLIDERUI_SRC=${1:-}
TOOLCHAIN_REPO_DIR=$(pwd)
WORKSPACE_DIR="$TOOLCHAIN_REPO_DIR/workspace"

if [ -n "$SLIDERUI_SRC" ]; then
  echo "[build] Copying sliderUI source from $SLIDERUI_SRC into workspace..."
  rm -rf "$WORKSPACE_DIR/sliderUI"
  mkdir -p "$WORKSPACE_DIR"
  cp -r "$SLIDERUI_SRC" "$WORKSPACE_DIR/sliderUI"
fi

IMAGE_NAME="sliderui_union_toolchain"
if [ -f Dockerfile ]; then
  echo "[build] Building toolchain image locally as $IMAGE_NAME"
  docker build -t "$IMAGE_NAME" .
else
  IMAGE_NAME="shauninman/union-miyoomini-toolchain:latest"
  docker pull "$IMAGE_NAME" || { echo "Failed to pull toolchain image."; exit 2; }
fi

echo "[build] Running container to build sliderUI and installer..."
docker run --rm -v "$WORKSPACE_DIR":/root/workspace -w /root/workspace/sliderUI "$IMAGE_NAME" /bin/bash -lc "\
set -e; \
echo '[container] pwd:' \$(pwd); \
make clean || true; \
make -j\$(nproc) all \
"

echo "[build] Build finished. Artifacts in: $WORKSPACE_DIR/sliderUI/build"
ls -l "$WORKSPACE_DIR/sliderUI/build"
cat "$WORKSPACE_DIR/sliderUI/build/"*.sha256 || true
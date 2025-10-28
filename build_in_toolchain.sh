#!/usr/bin/env bash
set -euo pipefail

SLIDERUI_SRC=${1:-.}
TOOLCHAIN_REPO_DIR=$(pwd)
WORKSPACE_DIR="$TOOLCHAIN_REPO_DIR/workspace"

# If user passed a directory, refresh code inside workspace
if [ -n "$SLIDERUI_SRC" ]; then
  echo "[build] Copying sliderUI source from $SLIDERUI_SRC into workspace..."
  rm -rf "$WORKSPACE_DIR/sliderUI"
  mkdir -p "$WORKSPACE_DIR"
  cp -r "$SLIDERUI_SRC" "$WORKSPACE_DIR/sliderUI"
fi

# ✅ Always use the LOCAL toolchain image
IMAGE_NAME="union-miyoomini-toolchain:latest"

echo "[build] Using local toolchain image: $IMAGE_NAME"

echo "[build] Running container to build sliderUI and installer..."
docker run --rm \
  -u "$(id -u):$(id -g)" \
  -v "$WORKSPACE_DIR":/root/workspace \
  -w /root/workspace/sliderUI \
  "$IMAGE_NAME" \
  /bin/bash -lc "\
set -e; \
echo '[container] pwd:' \$(pwd); \
source /root/.bashrc || true; \
make clean || true; \
make -j\$(nproc) all \
"

echo "[build] Build finished. Artifacts in: $WORKSPACE_DIR/sliderUI/build"
ls -l "$WORKSPACE_DIR/sliderUI/build"
cat "$WORKSPACE_DIR/sliderUI/build/"*.sha256 || true

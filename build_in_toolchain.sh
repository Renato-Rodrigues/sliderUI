#!/usr/bin/env bash
set -euo pipefail

# Get the absolute path to the project directory (where this script is)
PROJECT_ROOT=$(pwd)

# --- No more copying ---
# The old logic of copying source into a 'workspace' subdirectory was
# causing recursive copy errors and permission issues.
# Instead, we will mount the entire project directory directly.

# ✅ Always use the LOCAL toolchain image
IMAGE_NAME="union-miyoomini-toolchain:latest"

echo "[build] Using local toolchain image: $IMAGE_NAME"
echo "[build] Mounting project directory: $PROJECT_ROOT"

# --- One-time cleanup ---
# If you are getting "Permission denied" errors from a previous build,
# you must run 'sudo rm -rf workspace/' ONCE from this directory
# to remove the old root-owned files. This script will no longer use
# or create that 'workspace' directory.

echo "[build] Running container to build sliderUI and installer..."

# We construct the entire build command as a single line string to avoid
# multi-line parsing issues by the host shell.
# We explicitly call 'pwd' and 'nproc' inside the container via escaped $()
BUILD_COMMAND="set -e; echo '[container] In directory: \$(pwd) (Expected: /app)'; echo '[container] Running make clean...'; make clean || true; echo '[container] Running make all...'; make -j\$(nproc) all"

# Run the container:
# - Mount the current project directory (PROJECT_ROOT) to /app inside the container
# - Set the working directory inside the container to /app
# - Run as the host user ("$(id -u):$(id -g)") to avoid file permission problems
docker run --rm \
  -u "$(id -u):$(id -g)" \
  -v "$PROJECT_ROOT":/app \
  -w /app \
  "$IMAGE_NAME" \
  /bin/bash -c "$BUILD_COMMAND"

echo "[build] Build finished. Artifacts in: $PROJECT_ROOT/build"
ls -l "$PROJECT_ROOT/build"
cat "$PROJECT_ROOT/build/"*.sha256 || true

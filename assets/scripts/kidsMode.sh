#!/bin/sh
# kidsMode.sh
# Safely enable "kids mode" auto_resume for sliderUI on the Miyoo.
# Usage: run as regular user on the device; uses absolute paths.

ROOT="/mnt/SDCARD"
TOOLS_DIR="$ROOT/tools/sliderUI"
MINUI_SHARED="$ROOT/.userdata/shared/.minui"
AUTO_RESUME="$MINUI_SHARED/auto_resume.txt"
KIDS_SOURCE="$TOOLS_DIR/kids_mode_auto_resume.txt"
ENABLE_SIMPLE_FLAG="$ROOT/.userdata/shared/enable-simple-mode"
BACKUP_SUFFIX=".bak"

log() { printf '%s\n' "$1" >&2; }

# Ensure source exists
if [ ! -f "$KIDS_SOURCE" ]; then
  log "ERROR: kids_mode_auto_resume.txt not found at: $KIDS_SOURCE"
  exit 2
fi

# Ensure shared dir exists (create if needed)
if [ ! -d "$MINUI_SHARED" ]; then
  # create directory tree with safe permissions
  if ! mkdir -p "$MINUI_SHARED" 2>/dev/null; then
    log "ERROR: cannot create directory: $MINUI_SHARED"
    exit 3
  fi
fi

# Backup existing auto_resume file if present
if [ -f "$AUTO_RESUME" ]; then
  TS=$(date +%Y%m%d_%H%M%S 2>/dev/null || printf "%s" "$(date +%s)")
  BACKUP="$MINUI_SHARED/auto_resume.txt${BACKUP_SUFFIX}.${TS}"
  if ! mv -f "$AUTO_RESUME" "$BACKUP" 2>/dev/null; then
    log "WARNING: failed to backup existing auto_resume to $BACKUP (continuing)"
  else
    log "Backed up existing auto_resume to: $BACKUP"
  fi
fi

# Install kids_mode_auto_resume.txt atomically: copy to tmp then move
TMP_DEST="$MINUI_SHARED/auto_resume.txt.tmp.$$"
if cp -f "$KIDS_SOURCE" "$TMP_DEST" 2>/dev/null; then
  if mv -f "$TMP_DEST" "$AUTO_RESUME" 2>/dev/null; then
    log "Installed kids auto_resume at: $AUTO_RESUME"
  else
    log "ERROR: failed to move $TMP_DEST -> $AUTO_RESUME"
    rm -f "$TMP_DEST" 2>/dev/null
    exit 4
  fi
else
  log "ERROR: failed to copy $KIDS_SOURCE -> $TMP_DEST"
  rm -f "$TMP_DEST" 2>/dev/null
  exit 5
fi

# Create enable-simple-mode flag file
ENABLE_DIR=$(dirname "$ENABLE_SIMPLE_FLAG")
if [ ! -d "$ENABLE_DIR" ]; then
  if ! mkdir -p "$ENABLE_DIR" 2>/dev/null; then
    log "ERROR: cannot create directory for enable-simple-mode: $ENABLE_DIR"
    exit 6
  fi
fi

if ! touch "$ENABLE_SIMPLE_FLAG" 2>/dev/null; then
  log "ERROR: failed to create enable-simple-mode flag at: $ENABLE_SIMPLE_FLAG"
  exit 7
fi

log "Kids mode installed successfully."
exit 0

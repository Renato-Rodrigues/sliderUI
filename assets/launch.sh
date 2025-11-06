#!/bin/sh
# launch.sh
# Make sure the bundled elfs are executable and exec the menu ELF.
# Usage: placed in /mnt/SDCARD/tools/sliderUI/launch.sh

# Resolve script dir (POSIX)
SCRIPT_DIR="$(
  cd "$(dirname "$0")" 2>/dev/null || exit 1
  pwd -P
)"

MENU_ELF="$SCRIPT_DIR/menu.elf"
SLIDER_ELF="$SCRIPT_DIR/sliderUI.elf"

# Ensure files exist
if [ ! -f "$MENU_ELF" ]; then
  printf 'ERROR: %s not found\n' "$MENU_ELF" >&2
  exit 2
fi
if [ ! -f "$SLIDER_ELF" ]; then
  printf 'WARNING: %s not found (sliderUI may be optional)\n' "$SLIDER_ELF" >&2
fi

# Make them executable (ignore failures)
chmod +x "$MENU_ELF" 2>/dev/null || true
chmod +x "$SLIDER_ELF" 2>/dev/null || true

# Exec menu.elf (replace shell)
exec "$MENU_ELF"
# If exec fails, print error and exit
printf 'ERROR: failed to exec %s\n' "$MENU_ELF" >&2
exit 3

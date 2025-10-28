# Makefile for sliderUI (main app + SDL installer + bundle)
# Designed to run inside union-miyoomini-toolchain container (non-interactive)
# Usage:
#   make            # builds both sliderUI and sliderUI_installer
#   make clean
#   make installer  # builds only installer
#   make app        # builds only main app
#   make checksums  # create sha256 sums
#   make bundle     # bundle .so dependencies into deploy/lib (requires sysroot in TOOLCHAIN)
#   make install-wrapper  # create run_sliderUI.sh in deploy/
#   make deploy SD=/media/sdcard   # copy built files to SD card (host should run this)

# Compiler selection (toolchain container sets appropriate PATH so `g++` resolves to cross-compiler)
CXX ?= $(shell which g++ 2>/dev/null || echo g++)
CXXFLAGS = -O2 -Wall -std=c++17 -I/usr/include/SDL2 -D_REENTRANT
LDFLAGS = -lSDL2 -lSDL2_image -lSDL2_ttf

SRCDIR = src
BUILDDIR = build
DEPLOYDIR = deploy
LIB_DIR = $(DEPLOYDIR)/lib
ASSETS = assets
TOOLS = tools
DATA = data
CONFIG = config

APP_SRC = $(SRCDIR)/main.cpp $(SRCDIR)/slider.cpp $(SRCDIR)/reflection_cache.cpp $(SRCDIR)/dat_cache.cpp
APP = $(BUILDDIR)/sliderUI

INSTALLER_SRC = $(SRCDIR)/slider_installer.cpp
INSTALLER = $(BUILDDIR)/sliderUI_installer

# Toolchain/sysroot path used for bundling libraries. Adjust if your toolchain uses another path inside container.
TOOLCHAIN := /opt/union_toolchain
SYSROOT := $(TOOLCHAIN)/sysroot

.PHONY: all clean app installer checksums bundle install-wrapper deploy check-sd

all: app installer checksums

app: $(APP)

installer: $(INSTALLER)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(APP): $(BUILDDIR) $(APP_SRC)
	@echo "[make] Building main app -> $(APP)"
	$(CXX) $(CXXFLAGS) -o $(APP) $(APP_SRC) $(LDFLAGS)

$(INSTALLER): $(BUILDDIR) $(INSTALLER_SRC)
	@echo "[make] Building installer -> $(INSTALLER)"
	$(CXX) $(CXXFLAGS) -o $(INSTALLER) $(INSTALLER_SRC) $(LDFLAGS)

checksums: $(APP) $(INSTALLER)
	sha256sum $(APP) > $(APP).sha256
	sha256sum $(INSTALLER) > $(INSTALLER).sha256
	@echo "[make] Created checksums in $(BUILDDIR)/"

clean:
	rm -rf $(BUILDDIR) $(DEPLOYDIR)
	@echo "[make] Clean done."

# bundle - copy target ELF's NEEDED .so from toolchain sysroot to deploy/lib/
bundle: $(APP)
	@echo "===> Preparing deploy directory..."
	mkdir -p $(LIB_DIR)
	@echo "===> Locating required shared libraries listed by readelf..."
	# readelf may live in toolchain bin folder; try fallback to readelf in PATH
	READelf := $(shell if [ -x "$(TOOLCHAIN)/bin/readelf" ]; then echo "$(TOOLCHAIN)/bin/readelf"; elif command -v readelf >/dev/null 2>&1; then echo "readelf"; else echo ""; fi)
	@if [ -z "$(READelf)" ]; then echo "Error: readelf not found in toolchain or PATH."; exit 1; fi
	@NEEDED_LIBS=`$(READelf) -d $(APP) | grep NEEDED | sed -e 's/.*\\[\\(.*\\)\\].*/\\1/'` ; \
	if [ -z "$$NEEDED_LIBS" ]; then echo "No NEEDED libs found or error reading ELF."; fi ; \
	for LIB in $$NEEDED_LIBS; do \
		# try finding first in sysroot then in common lib dirs
		LIBPATH=`find $(SYSROOT) -name $$LIB 2>/dev/null | head -n 1`; \
		if [ -z "$$LIBPATH" ]; then \
			LIBPATH=`find $(SYSROOT) -name "*$$LIB*" 2>/dev/null | head -n 1`; \
		fi; \
		if [ -n "$$LIBPATH" ]; then \
			echo "Copying $$LIB from $$LIBPATH"; \
			cp -u "$$LIBPATH" $(LIB_DIR)/; \
		else \
			echo "Warning: $$LIB not found in sysroot ($(SYSROOT)). You may need to adjust TOOLCHAIN/SYSROOT."; \
		fi; \
	done
	@echo "===> Bundle complete. Deploy directory: $(DEPLOYDIR)/"

# create run wrapper to set LD_LIBRARY_PATH
install-wrapper:
	mkdir -p $(DEPLOYDIR)
	cat > $(DEPLOYDIR)/run_sliderUI.sh <<'EOF'
#!/bin/sh
# run_sliderUI.sh - wrapper that sets LD_LIBRARY_PATH to bundled libs (deploy/lib)
APPDIR="$(dirname "$0")"
# Prepend app lib to LD_LIBRARY_PATH
export LD_LIBRARY_PATH="$APPDIR/lib:$LD_LIBRARY_PATH"
# Useful SDL env overrides
export SDL_AUDIODRIVER="alsa"
# Launch the binary (assumes binary sits next to the wrapper)
cd "$APPDIR"
exec ./sliderUI
EOF
	chmod +x $(DEPLOYDIR)/run_sliderUI.sh
	@echo "[make] Created wrapper: $(DEPLOYDIR)/run_sliderUI.sh"

# deploy on host to SD card (run from host)
deploy: check-sd
	@echo "[make] Deploying to SD card at $(SD)"
	mkdir -p "$(SD)/App/sliderUI_installer"
	mkdir -p "$(SD)/App/sliderUI"
	# copy installer
	cp -v $(INSTALLER) "$(SD)/App/sliderUI_installer/sliderUI_installer"
	chmod +x "$(SD)/App/sliderUI_installer/sliderUI_installer"
	# copy installer resources
	@if [ -d "$(ASSETS)" ]; then cp -r $(ASSETS) "$(SD)/App/sliderUI_installer/" || true; fi
	@if [ -d "$(DATA)" ]; then cp -r $(DATA) "$(SD)/App/sliderUI_installer/" || true; fi
	@if [ -d "$(CONFIG)" ]; then cp -r $(CONFIG) "$(SD)/App/sliderUI_installer/" || true; fi
	@if [ -d "$(TOOLS)" ]; then cp -r $(TOOLS) "$(SD)/App/sliderUI_installer/" || true; fi
	# installer metadata
	cat > "$(SD)/App/sliderUI_installer/metadata.txt" <<EOF
title=SliderUI Installer
description=Install SliderUI from the MinUI menu (no terminal)
exec=/mnt/SDCARD/App/sliderUI_installer/sliderUI_installer
EOF
	# copy main app (optional) and wrapper/bundle if present
	if [ -f "$(APP)" ]; then \
		cp -v $(APP) "$(SD)/App/sliderUI/sliderUI"; chmod +x "$(SD)/App/sliderUI/sliderUI"; \
	fi
	if [ -d "$(DEPLOYDIR)" ]; then \
		cp -r $(DEPLOYDIR)/* "$(SD)/App/sliderUI/" || true; \
	fi
	# main app metadata
	cat > "$(SD)/App/sliderUI/metadata.txt" <<EOF
title=Slider Mode
description=Kid-friendly slider UI (direct run)
exec=/mnt/SDCARD/App/sliderUI/run_sliderUI.sh
EOF
	@echo "[make] Deploy complete. Remember to safely eject the SD card."

# check-sd ensures SD var set and path exists
check-sd:
ifndef SD
	$(error SD not set. Usage: make deploy SD=/media/sdcard)
endif
	@if [ ! -d "$(SD)" ]; then echo "SD mount $(SD) does not exist."; exit 1; fi

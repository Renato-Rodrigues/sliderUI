# Makefile for sliderUI (main app + SDL installer + bundle)
# Designed to run inside union-miyoomini-toolchain container

# Toolchain paths
TOOLCHAIN := /opt/miyoomini-toolchain
SYSROOT := $(TOOLCHAIN)/arm-linux-gnueabihf/libc

# Compiler selection - use the toolchain's cross-compiler
CXX := $(TOOLCHAIN)/bin/arm-linux-gnueabihf-g++
CC := $(TOOLCHAIN)/bin/arm-linux-gnueabihf-gcc

# Compiler flags with sysroot includes
CXXFLAGS = -O2 -Wall -std=c++17 -D_REENTRANT \
	-I$(SYSROOT)/usr/include \
	-I$(SYSROOT)/usr/include/SDL \
	-I$(SYSROOT)/usr/include/arm-linux-gnueabihf

LDFLAGS = -L$(SYSROOT)/usr/lib \
	-L$(SYSROOT)/usr/lib/arm-linux-gnueabihf \
	-lSDL -lSDL_image -lSDL_ttf -lstdc++fs -lpthread

SRCDIR = src
BUILDDIR = build
DEPLOYDIR = deploy
LIB_DIR = $(DEPLOYDIR)/lib

APP_SRC = $(SRCDIR)/main.cpp $(SRCDIR)/slider.cpp $(SRCDIR)/reflection_cache.cpp $(SRCDIR)/dat_cache.cpp $(SRCDIR)/stb_image_write.cpp
APP = $(BUILDDIR)/sliderUI

INSTALLER_SRC = $(SRCDIR)/slider_installer.cpp
INSTALLER = $(BUILDDIR)/sliderUI_installer

READELF := $(TOOLCHAIN)/bin/arm-linux-gnueabihf-readelf

.PHONY: all clean app installer checksums bundle install-wrapper verify_sdl

all: app installer checksums bundle install-wrapper verify_sdl

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

bundle: $(APP)
	@echo "===> Preparing deploy directory..."
	@mkdir -p $(LIB_DIR)
	@echo "===> Locating required shared libraries..."
	NEEDED_LIBS=`$(READELF) -d $(APP) | grep NEEDED | sed -e 's/.*\[\(.*\)\].*/\1/'`; \
	for LIB in $$NEEDED_LIBS; do \
		LIBPATH=`find $(SYSROOT)/usr/lib -name "$$LIB" 2>/dev/null | head -n 1`; \
		if [ -z "$$LIBPATH" ]; then \
			LIBPATH=`find $(SYSROOT)/lib -name "$$LIB" 2>/dev/null | head -n 1`; \
		fi; \
		if [ -n "$$LIBPATH" ]; then \
			echo "Copying $$LIB from $$LIBPATH"; \
			cp "$$LIBPATH" $(LIB_DIR)/; \
		else \
			echo "Warning: $$LIB not found in sysroot"; \
		fi; \
	done
	@echo "===> Bundle complete."

install-wrapper:
	@mkdir -p $(DEPLOYDIR)
	@printf '#!/bin/sh\n' > $(DEPLOYDIR)/run_sliderUI.sh
	@printf 'APPDIR="$$(dirname "$$0")"\n' >> $(DEPLOYDIR)/run_sliderUI.sh
	@printf 'export LD_LIBRARY_PATH="$$APPDIR/lib:$$LD_LIBRARY_PATH"\n' >> $(DEPLOYDIR)/run_sliderUI.sh
	@printf 'export SDL_NOMOUSE=1\n' >> $(DEPLOYDIR)/run_sliderUI.sh
	@printf 'cd "$$APPDIR"\n' >> $(DEPLOYDIR)/run_sliderUI.sh
	@printf 'exec ./sliderUI\n' >> $(DEPLOYDIR)/run_sliderUI.sh
	@chmod +x $(DEPLOYDIR)/run_sliderUI.sh
	@echo "[make] Created wrapper: $(DEPLOYDIR)/run_sliderUI.sh"

verify_sdl:
	@echo "[make] Verifying SDL dependencies..."
	@$(READELF) -V >/dev/null 2>&1 || echo "Warning: readelf not found or not executable"
	@echo "Expected: libSDL-1.2.so.0 (NOT libSDL2)"

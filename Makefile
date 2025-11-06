# Compiler and flags
CXX ?= g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Iinclude
LDFLAGS :=

# Build type and platform
BUILD_TYPE ?= linux

# Detect SDL 1.2
SDL_CFLAGS := $(shell sdl-config --cflags)
SDL_LIBS := $(shell sdl-config --libs) -lSDL_ttf -lSDL_image

# Project paths
BIN_DIR := bin
CORE_SRC := $(wildcard src/core/*.cpp)
UI_BASE_SRC := $(filter-out src/ui/menu_main.cpp src/ui/slider_main.cpp src/ui/renderer_*.cpp,$(wildcard src/ui/*.cpp))
CORE_OBJS := $(filter-out src/core/renderer_stub.o,$(CORE_SRC:.cpp=.o))
# MENU_UI_OBJS and SLIDER_UI_OBJS will be defined after build.mk is included

# Build configuration
ifeq ($(BUILD_TYPE),desktop)
  CXXFLAGS += $(SDL_CFLAGS) -DUSE_SDL_PREVIEW
  LDFLAGS += $(SDL_LIBS)
  RENDERER_SRC := src/ui/renderer_sdl.cpp
  MENU_BIN := $(BIN_DIR)/menu.elf
  SLIDER_BIN := $(BIN_DIR)/sliderUI.elf
  EXTRA_OBJS :=
  $(info Building desktop SDL preview version...)
else
  CXXFLAGS += -Isrc/minui
  RENDERER_SRC := src/ui/renderer_minui.cpp
  MENU_BIN := menu.elf
  SLIDER_BIN := sliderUI.elf
  EXTRA_OBJS := src/minui/platform_minui.o
  $(info Building target MinUI version...)
endif

# Build type detection (desktop/linux/myioo)
BUILD_TYPE ?= linux

# Build type specific configuration
ifeq ($(BUILD_TYPE),desktop)
  # SDL 1.2 configuration for desktop preview
  SDL_CFLAGS := $(shell sdl-config --cflags 2>/dev/null)
  SDL_LIBS := $(shell sdl-config --libs 2>/dev/null) -lSDL_ttf -lSDL_image
  ifeq ($(SDL_CFLAGS),)
    $(error SDL 1.2 development packages not found. Please install: sudo apt-get install libsdl1.2-dev libsdl-ttf2.0-dev libsdl-image1.2-dev)
  endif
  RENDERER_SRC := src/ui/renderer_sdl.cpp
  RENDERER_FLAGS := $(SDL_CFLAGS) -DUSE_SDL_PREVIEW
  CXXFLAGS += $(SDL_CFLAGS) -DUSE_SDL_PREVIEW
  LDFLAGS += $(SDL_LIBS)
  EXTRA_OBJS :=
else
  # Use MinUI renderer for target builds
  RENDERER_SRC := src/ui/renderer_minui.cpp
  RENDERER_FLAGS := -Isrc/minui
  EXTRA_OBJS := src/minui/platform_minui.o
endif

# Set renderer object file
RENDERER_OBJ := $(RENDERER_SRC:.cpp=.o)

# detect libwebp with pkg-config on host
WEBP_CFLAGS := $(shell pkg-config --cflags libwebp 2>/dev/null)
WEBP_LIBS   := $(shell pkg-config --libs libwebp 2>/dev/null)

# for cross builds: allow overriding with MYIOO_WEBP_PREFIX
ifdef MYIOO_WEBP_PREFIX
  WEBP_CFLAGS := -I$(MYIOO_WEBP_PREFIX)/include
  WEBP_LIBS   := -L$(MYIOO_WEBP_PREFIX)/lib -lwebp
endif

# enable HAVE_WEBP if we have libs (or force with ENABLE_WEBP=1)
ifeq ($(strip $(WEBP_LIBS)),)
  ifndef ENABLE_WEBP
    WEBP_ENABLED := 0
  else
    WEBP_ENABLED := 1
  endif
else
  WEBP_ENABLED := 1
endif

# Basic compiler flags
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Iinclude $(WEBP_FLAGS)

# Add MinUI include path for non-desktop builds
ifneq ($(BUILD_TYPE),desktop)
  CXXFLAGS += -Isrc/minui
endif

# Add SDL 1.2 flags for desktop builds
ifeq ($(BUILD_TYPE),desktop)
  CXXFLAGS := $(filter-out -Isrc/minui,$(CXXFLAGS))
  CXXFLAGS += $(SDL_CFLAGS) -DUSE_SDL_PREVIEW
  LDFLAGS += $(SDL_LIBS)
endif

# Add webp flags after other flags are configured
ifeq ($(WEBP_ENABLED),1)
  CXXFLAGS += -DHAVE_WEBP $(WEBP_CFLAGS)
  LDFLAGS  += $(WEBP_LIBS)
  $(info libwebp enabled)
else
  $(info libwebp disabled; compile will not include webp decoder)
endif

# Source files
CORE_SRC := $(wildcard src/core/*.cpp)
UI_BASE_SRC := $(filter-out src/ui/menu_main.cpp src/ui/slider_main.cpp src/ui/renderer_*.cpp,$(wildcard src/ui/*.cpp))

# Select renderer implementation based on build type
ifeq ($(BUILD_TYPE),desktop)
  RENDERER_SRC := src/ui/renderer_sdl.cpp
  # Remove MinUI include path for desktop builds
  CXXFLAGS := $(filter-out -Isrc/minui,$(CXXFLAGS))
else
  RENDERER_SRC := src/ui/renderer_minui.cpp
endif

CORE_OBJS := $(CORE_SRC:.cpp=.o)
UI_BASE_OBJS := $(UI_BASE_SRC:.cpp=.o)
RENDERER_OBJ := $(RENDERER_SRC:.cpp=.o)
UI_OBJS := $(UI_BASE_OBJS) $(RENDERER_OBJ)

# Adjust UI_OBJS to include only the appropriate renderer object
ifeq ($(BUILD_TYPE),desktop)
  UI_OBJS := $(filter-out src/ui/renderer_minui.o, $(UI_BASE_OBJS)) src/ui/renderer_sdl.o
else
  UI_OBJS := $(filter-out src/ui/renderer_sdl.o, $(UI_BASE_OBJS)) src/ui/renderer_minui.o
endif

# Platform implementation (only for non-desktop builds)
ifeq ($(BUILD_TYPE),desktop)
  PLATFORM_SRC :=
  PLATFORM_OBJ :=
  EXTRA_OBJS := $(RENDERER_OBJ)
else
  PLATFORM_SRC := src/minui/platform_minui.c
  PLATFORM_OBJ := $(PLATFORM_SRC:.c=.o)
  EXTRA_OBJS := $(PLATFORM_OBJ) $(RENDERER_OBJ)
endif

# Include build configuration
include build.mk

# Define menu and slider UI objects after UI_COMMON_OBJS is defined
MENU_UI_OBJS := src/ui/menu_main.o $(UI_COMMON_OBJS)
SLIDER_UI_OBJS := src/ui/slider_main.o src/ui/slider_ui.o $(UI_COMMON_OBJS)

# Base objects for linking (excluding stub)
CORE_OBJS := $(filter-out src/core/renderer_stub.o,$(CORE_SRC:.cpp=.o))

# Combined objects for linking
LINK_OBJS := $(CORE_OBJS) $(UI_COMMON_OBJS) $(EXTRA_OBJS)

# Filter out any stub implementations
CORE_OBJS := $(filter-out src/core/renderer_stub.o,$(CORE_OBJS))

# Output binaries
ifeq ($(BUILD_TYPE),desktop)
  BIN_DIR := bin
  MENU_BIN := $(BIN_DIR)/menu.elf
  SLIDER_BIN := $(BIN_DIR)/sliderUI.elf
else
  MENU_BIN := menu.elf
  SLIDER_BIN := sliderUI.elf
endif

# Tests
TEST_SRCS := $(wildcard test/*.cpp)
TEST_BINS := $(patsubst test/%.cpp,test/bin/%,$(TEST_SRCS))

.PHONY: all linux myioo desktop clean test test_run info run run_slider run_all prepare release

all: linux

prepare:
	@mkdir -p $(BIN_DIR) test/bin

# Desktop preview build
desktop:
	@$(MAKE) BUILD_TYPE=desktop CXX=g++ prepare build_common bin/menu.elf bin/sliderUI.elf

# Local linux build (fast)
linux: CXX := g++
linux: BUILD_TYPE := linux
linux: build_common $(MENU_BIN) $(SLIDER_BIN)

# Cross (myioo) build
myioo: BUILD_TYPE := myioo
myioo: build_common $(MENU_BIN) $(SLIDER_BIN)

# Common object compilation
build_common: $(CORE_OBJS)

# Link menu
$(MENU_BIN): build_common $(MENU_UI_OBJS) $(RENDERER_OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $(CORE_OBJS) $(MENU_UI_OBJS) $(EXTRA_OBJS) $(LDFLAGS)

# Link slider
$(SLIDER_BIN): build_common $(SLIDER_UI_OBJS) $(RENDERER_OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $(CORE_OBJS) $(SLIDER_UI_OBJS) $(EXTRA_OBJS) $(LDFLAGS)

# Special compile rule for renderer
$(RENDERER_OBJ): $(RENDERER_SRC)
	$(CXX) $(CXXFLAGS) $(RENDERER_FLAGS) -c $< -o $@

# Generic compile rule
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Tests (built with host compiler)
TEST_CXX := g++
TEST_CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Iinclude

test/bin/%: test/%.cpp | test/bin build_common
	$(TEST_CXX) $(TEST_CXXFLAGS) -o $@ $(CORE_OBJS) $(UI_OBJS) $<

test: $(TEST_BINS)
	@echo "[test] Running $(words $(TEST_BINS)) tests..."
	@$(MAKE) test_run

test_run:
	@set -e; \
	for tb in $(TEST_BINS); do \
	  echo "[test] Running $$tb"; \
	  ./$$tb || { echo "[test] FAIL: $$tb"; exit 1; }; \
	done; \
	echo "[test] All tests passed."

# Run targets
run: desktop
	@echo "[run] Executing ./$(MENU_BIN)"
	@./$(MENU_BIN)

run_slider: desktop
	@echo "[run] Executing ./$(SLIDER_BIN)"
	@./$(SLIDER_BIN)

run_all: run run_slider

clean:
	rm -rf $(BIN_DIR) test/bin *.o src/core/*.o src/ui/*.o release

release: desktop
	@echo "[release] Creating release package..."
	@rm -rf release
	@mkdir -p release/bin release/res release/resources/fonts
	@echo "[release] Copying binaries..."
	@cp $(BIN_DIR)/menu.elf release/bin/
	@cp $(BIN_DIR)/sliderUI.elf release/bin/
	@echo "[release] Copying resources..."
	@cp -r res/* release/res/ 2>/dev/null || true
	@cp -r resources/* release/resources/ 2>/dev/null || true
	@if [ -f sliderUI_cfg.json ]; then cp sliderUI_cfg.json release/; fi
	@if [ -f gameList.csv ]; then cp gameList.csv release/; fi
	@echo "[release] Creating README..."
	@echo "SliderUI Release Package" > release/README.txt
	@echo "=======================" >> release/README.txt
	@echo "" >> release/README.txt
	@echo "This package contains the compiled SliderUI application for desktop preview." >> release/README.txt
	@echo "" >> release/README.txt
	@echo "Contents:" >> release/README.txt
	@echo "- bin/          : Compiled binaries (menu.elf, sliderUI.elf)" >> release/README.txt
	@echo "- res/          : Application resources (fonts, images)" >> release/README.txt
	@echo "- resources/   : Additional resources" >> release/README.txt
	@echo "- sliderUI_cfg.json : Configuration file (if included)" >> release/README.txt
	@echo "- gameList.csv : Game list file (if included)" >> release/README.txt
	@echo "" >> release/README.txt
	@echo "Requirements:" >> release/README.txt
	@echo "- SDL 1.2 runtime libraries:" >> release/README.txt
	@echo "  * libsdl1.2 (SDL library)" >> release/README.txt
	@echo "  * libsdl-ttf2.0 (SDL TTF font library)" >> release/README.txt
	@echo "  * libsdl-image1.2 (SDL Image library)" >> release/README.txt
	@echo "" >> release/README.txt
	@echo "Installation on Ubuntu/Debian:" >> release/README.txt
	@echo "  sudo apt-get install libsdl1.2debian libsdl-ttf2.0-0 libsdl-image1.2" >> release/README.txt
	@echo "" >> release/README.txt
	@echo "Running:" >> release/README.txt
	@echo "  cd bin" >> release/README.txt
	@echo "  ./menu.elf      # Run menu interface" >> release/README.txt
	@echo "  ./sliderUI.elf  # Run slider interface" >> release/README.txt
	@echo "" >> release/README.txt
	@echo "Note: Make sure all required SDL libraries are installed before running." >> release/README.txt
	@echo "[release] Release package created in 'release/' directory"
	@echo "[release] Release contents:"
	@find release -type f | sort

info:
	@echo "Build configuration:"
	@echo "  BUILD_TYPE = $(BUILD_TYPE)"
	@echo "  CXX = $(CXX)"
	@echo "  CROSS_PREFIX = $(CROSS_PREFIX)"
	@echo "  WEBP_ENABLED = $(WEBP_ENABLED)"
	@echo ""
	@echo "Available targets:"
	@echo "  make desktop     # build SDL 1.2 preview version"
	@echo "  make linux      # build production Linux version"
	@echo "  make myioo      # build for Miyoo Mini"
	@echo "  make test       # run tests"
	@echo "  make run        # run preview menu"
	@echo "  make run_slider # run preview slider"
	@echo "  make release    # create release package"
	@echo ""
	@echo "For Miyoo builds, either:"
	@echo "  - Use the toolchain container: make myioo"
	@echo "  - Set CROSS_PREFIX: make myioo CROSS_PREFIX=mipsel-linux-gnu-"
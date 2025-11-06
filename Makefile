# Makefile (cleaned & fixed)
# Compiler and flags
CXX ?= g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Iinclude
LDFLAGS :=

# Build type and platform (default: linux)
BUILD_TYPE ?= linux

# Project paths
BIN_DIR := bin

# Detect SDL 1.2 only when needed (desktop build will re-run sdl-config)
SDL_CFLAGS := $(shell sdl-config --cflags 2>/dev/null)
SDL_LIBS   := $(shell sdl-config --libs 2>/dev/null) -lSDL_image -lSDL_ttf

# ALWAYS enable SDL_ttf symbol for compiling renderer code that uses TTF.
# (Linking will only succeed on desktop when SDL dev packages are present;
# for non-desktop targets we still define the symbol so code using #ifdef compiles)
CXXFLAGS += -DHAVE_SDL_TTF
LDFLAGS  += -lSDL_ttf

# detect libwebp with pkg-config on host (optional)
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

ifeq ($(WEBP_ENABLED),1)
  CXXFLAGS += -DHAVE_WEBP $(WEBP_CFLAGS)
  LDFLAGS  += $(WEBP_LIBS)
  $(info libwebp enabled)
else
  $(info libwebp disabled; compile will not include webp decoder)
endif

# Project source discovery
CORE_SRC := $(wildcard src/core/*.cpp)
UI_BASE_SRC := $(filter-out src/ui/menu_main.cpp src/ui/slider_main.cpp src/ui/renderer_*.cpp,$(wildcard src/ui/*.cpp))

# Renderer selection defaults; overwritten later depending on BUILD_TYPE
RENDERER_SRC := src/ui/renderer_minui.cpp
RENDERER_FLAGS :=

# Build-type specific configuration (desktop vs others)
ifeq ($(BUILD_TYPE),desktop)
  # Ensure SDL development packages exist
  ifeq ($(SDL_CFLAGS),)
    $(error SDL 1.2 development packages not found. Please install: sudo apt-get install libsdl1.2-dev libsdl-ttf2.0-dev libsdl-image1.2-dev)
  endif

  $(info Building desktop SDL preview version...)
  CXXFLAGS += $(SDL_CFLAGS) -DUSE_SDL_PREVIEW
  LDFLAGS  += $(SDL_LIBS)
  RENDERER_SRC := src/ui/renderer_sdl.cpp
  RENDERER_FLAGS := $(SDL_CFLAGS) -DUSE_SDL_PREVIEW

  BIN_DIR := bin
  MENU_BIN := $(BIN_DIR)/menu.elf
  SLIDER_BIN := $(BIN_DIR)/sliderUI.elf
  EXTRA_OBJS :=

else
  $(info Building target MinUI version...)
  CXXFLAGS += -Isrc/minui
  RENDERER_SRC := src/ui/renderer_minui.cpp
  RENDERER_FLAGS := -Isrc/minui

  BIN_DIR :=
  MENU_BIN := menu.elf
  SLIDER_BIN := sliderUI.elf
  EXTRA_OBJS := src/minui/platform_minui.o
endif

# Objects and lists
CORE_OBJS := $(filter-out src/core/renderer_stub.o,$(CORE_SRC:.cpp=.o))
UI_BASE_OBJS := $(UI_BASE_SRC:.cpp=.o)
RENDERER_OBJ := $(RENDERER_SRC:.cpp=.o)

# UI objects that include renderer object
ifeq ($(BUILD_TYPE),desktop)
  UI_OBJS := $(filter-out src/ui/renderer_minui.o,$(UI_BASE_OBJS)) src/ui/renderer_sdl.o
else
  UI_OBJS := $(filter-out src/ui/renderer_sdl.o,$(UI_BASE_OBJS)) src/ui/renderer_minui.o
endif

# Platform object for non-desktop
ifeq ($(BUILD_TYPE),desktop)
  PLATFORM_OBJ :=
else
  PLATFORM_SRC := src/minui/platform_minui.c
  PLATFORM_OBJ := $(PLATFORM_SRC:.c=.o)
endif

# Extra objects for linking
ifeq ($(BUILD_TYPE),desktop)
  EXTRA_OBJS := $(RENDERER_OBJ)
else
  EXTRA_OBJS := $(PLATFORM_OBJ) $(RENDERER_OBJ)
endif

# Include optional build.mk if present (keep existing behaviour)
-include build.mk

# UI common objects (build.mk may define UI_COMMON_OBJS; fallback to UI_OBJS)
ifdef UI_COMMON_OBJS
  MENU_UI_OBJS := src/ui/menu_main.o $(UI_COMMON_OBJS)
  SLIDER_UI_OBJS := src/ui/slider_main.o src/ui/slider_ui.o $(UI_COMMON_OBJS)
else
  MENU_UI_OBJS := src/ui/menu_main.o $(UI_OBJS)
  SLIDER_UI_OBJS := src/ui/slider_main.o src/ui/slider_ui.o $(UI_OBJS)
endif

# Combined link objects
LINK_OBJS := $(CORE_OBJS) $(UI_OBJS) $(EXTRA_OBJS)

# Tests
TEST_SRCS := $(wildcard test/*.cpp)
TEST_BINS := $(patsubst test/%.cpp,test/bin/%,$(TEST_SRCS))

.PHONY: all linux myioo desktop clean test test_run info run run_slider run_all prepare release

all: linux

prepare:
	@mkdir -p $(BIN_DIR) test/bin

# Desktop preview build
desktop:
	@$(MAKE) BUILD_TYPE=desktop CXX=$(CXX) prepare build_common bin/menu.elf bin/sliderUI.elf

# Local linux build (fast)
linux: CXX := $(CXX)
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

# Special compile rule for renderer (use RENDERER_FLAGS)
$(RENDERER_OBJ): $(RENDERER_SRC)
	$(CXX) $(CXXFLAGS) $(RENDERER_FLAGS) -c $< -o $@

# Generic compile rule
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# C object rule (for platform_minui.c)
%.o: %.c
	$(CC) -c $< -o $@

# Tests (built with host compiler)
TEST_CXX := $(CXX)
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
	@mkdir -p release/cfg release/scripts release/assets/img release/assets/ctrl release/assets/font release/logs
	@echo "[release] Copying binaries..."
	@cp bin/menu.elf release/
	@cp bin/sliderUI.elf release/
	@echo "[release] Copying assets..."
	@cp -r  assets/* release/ 2>/dev/null || true
	@echo "[release] Release package created in 'release/' directory"

info:
	@echo "Build configuration:"
	@echo "  BUILD_TYPE = $(BUILD_TYPE)"
	@echo "  CXX = $(CXX)"
	@echo "  WEBP_ENABLED = $(WEBP_ENABLED)"
	@echo ""
	@echo "Available targets:"
	@echo "  make desktop     # build SDL 1.2 preview version"
	@echo "  make linux       # build production Linux version"
	@echo "  make myioo       # build for Miyoo Mini"
	@echo "  make test        # run tests"
	@echo "  make run         # run preview menu"
	@echo "  make run_slider  # run preview slider"
	@echo ""
	@echo "For Miyoo builds, either:"
	@echo "  - Use the toolchain container: make myioo"
	@echo "  - Set CROSS_PREFIX: make myioo CROSS_PREFIX=mipsel-linux-gnu-"

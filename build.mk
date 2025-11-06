# Source files
CORE_SRC := $(wildcard src/core/*.cpp)
UI_SRC := $(wildcard src/ui/*.cpp)

# Filter out main files and renderer implementations
UI_BASE_SRC := $(filter-out src/ui/menu_main.cpp src/ui/slider_main.cpp src/ui/renderer_*.cpp,$(UI_SRC))

# Common UI objects (includes shared components like menu_config)
UI_COMMON_OBJS := src/ui/menu_ui.o src/ui/games_list.o src/ui/menu_config.o

# Menu specific objects
MENU_UI_OBJS := src/ui/menu_main.o $(UI_COMMON_OBJS)

# Slider specific objects (also needs menu_config for screen dimensions)
SLIDER_UI_OBJS := src/ui/slider_main.o src/ui/slider_ui.o $(UI_COMMON_OBJS)
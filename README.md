# SliderUI

A user interface application designed for the Miyoo Mini gaming device, providing a smooth and intuitive game selection interface with additional features like kids mode and custom sorting options.

## 🎮 Features

- Smooth sliding UI for game selection
- Kids mode for restricted game access
- Configurable UI elements and behaviors
- CSV-based game list management
- Image caching for improved performance
- Custom sorting options
- Support for both Linux and Miyoo Mini platforms

## 📁 Project Structure

```
sliderUI/
├── buildAndRun.sh       # Build and run script
├── gameList.csv         # Sample game list
├── kidsMode.sh         # Kids mode management script
├── launch.sh           # Launch script
├── Makefile            # Build configuration
├── include/            # Header files
│   ├── core/          # Core functionality headers
│   │   ├── config_manager.h
│   │   ├── csv_parser.h
│   │   ├── file_utils.h
│   │   ├── game_db.h
│   │   ├── image_cache.h
│   │   ├── image_loader.h
│   │   ├── logger.h
│   │   └── sort.h
│   └── ui/            # UI-related headers
│       ├── menu_ui.h
│       └── renderer.h
├── src/               # Source files
│   ├── core/         # Core implementation
│   │   ├── config_manager.cpp
│   │   ├── csv_parser.cpp
│   │   ├── file_utils.cpp
│   │   ├── game_db.cpp
│   │   ├── image_cache.cpp
│   │   ├── image_loader.cpp
│   │   ├── logger.cpp
│   │   └── sort.cpp
│   └── ui/          # UI implementation
│       ├── menu_main.cpp
│       ├── menu_ui.cpp
│       ├── slider_main.cpp
│       └── slider_ui.cpp
└── test/            # Unit tests
    ├── test_config_manager.cpp
    ├── test_csv_parser.cpp
    ├── test_file_utils.cpp
    ├── test_game_db.cpp
    ├── test_image_cache.cpp
    ├── test_image_loader.cpp
    ├── test_logger.cpp
    └── test_sort.cpp
```

## 🔧 File Descriptions

### Core Components
- `config_manager`: JSON configuration management
- `csv_parser`: Game list CSV file parsing
- `file_utils`: File system operations
- `game_db`: Game database management
- `image_cache`: Image caching system
- `image_loader`: Image loading and processing
- `logger`: Logging system
- `sort`: Game list sorting algorithms

### UI Components
- `menu_ui`: Menu interface implementation
- `renderer`: Graphics rendering system
- `slider_ui`: Main sliding interface implementation
- `menu_main`, `slider_main`: Entry points for the applications

## 🚀 Building and Running

### Prerequisites

#### For Linux/WSL:
- g++ (with C++17 support)
- make
- bash
- dos2unix (recommended)
- ssh (optional)
- docker (optional, for Miyoo Mini toolchain)

#### For Miyoo Mini:
- MinUI environment
- union-miyoomini-toolchain

### Development Preview Mode

For UI development and testing, a preview mode using SDL 1.2 is available in Linux/WSL:

#### Prerequisites
```bash
# Install SDL 1.2 and dependencies
sudo apt-get update
sudo apt-get install libsdl1.2-dev libsdl-ttf2.0-dev libsdl-image1.2-dev
```

#### Building and Running Preview
```bash
# Build desktop preview version
make BUILD_TYPE=desktop

# Run the preview
./bin/sliderUI.elf    # For game browser interface
./bin/menu.elf        # For menu interface
```

#### Preview Controls
- Arrow keys: Navigation
- J key: A button
- K key: B button
- U key: X button
- I key: Y button
- M key: Menu button

#### Preview Features
- Real-time UI updates
- Accurate layout matching MinUI
- Image scaling and rendering
- Text rendering with shadows
- Game carousel visualization
- Performance monitoring

### Building for Linux Production

```bash
# Build for Linux
make linux

# Run the application
./buildAndRun.sh
```

### Building for Miyoo Mini

1. Set up the union-miyoomini-toolchain:
```bash
git clone https://github.com/shauninman/union-miyoomini-toolchain.git
cd union-miyoomini-toolchain
make shell
```

2. Inside the toolchain container:
```bash
cd /workspace/sliderUI
make myioo
```

### Deploying to Miyoo Mini

1. Create the target directory on your SD card:
```bash
SDROOT="/path/to/sd"
mkdir -p "$SDROOT/tools/sliderUI"
```

2. Copy the required files:
```bash
cp menu.elf sliderUI.elf "$SDROOT/tools/sliderUI/"
cp sliderUI_cfg.json gameList.csv "$SDROOT/tools/sliderUI/"
cp tools/kidsMode.sh tools/launch.sh "$SDROOT/tools/sliderUI/"
```

3. On the device:
```bash
cd /mnt/SDCARD/tools/sliderUI
chmod +x menu.elf sliderUI.elf
./menu.elf
```

## 🧪 Testing

The project includes comprehensive unit tests for all core components. To run the tests:

```bash
make test
./test_runner
```

## ⚙️ Configuration

The application can be configured through `sliderUI_cfg.json`. Key configuration options include:

### UI Configuration
```json
{
  "ui": {
    "resolution": [640, 480],
    "background": "bckg.png",
    "title": {
      "x": 20,
      "y": 40,
      "size": 28,
      "font": "default",
      "color": "#FFFFFF",
      "align": "left"
    },
    "game_image": {
      "x": 200,
      "y": 160,
      "width": 280,
      "height": 200,
      "margin": 40,
      "scale": "fit",
      "side_scale": 0.8
    }
  }
}
```

### Key Configuration Areas:
- UI appearance and positioning
- Text styling and colors
- Image scaling and placement
- Kids mode settings
- Image cache settings
- Sorting preferences
- Input handling
- Logging options

Configuration changes take effect immediately in preview mode, allowing for rapid UI iteration.

## 📋 Game List Format

Games are listed in `gameList.csv` with the following format:
```csv
path;order in custom sort mode;title,release date
/path/to/game.gba,0,Game Name,YYYY-MM-DD
```
- If Game name does not exist, the menu will use the game filename
- The release date accepts either YYYY-MM-DD, YYYY-MM or YYYY formats

## 🚨 Troubleshooting

- **Clock skew issues on WSL**: Use `sudo ntpdate -u time.google.com` or `wsl --shutdown`
- **Script errors**: Ensure Unix line endings with `dos2unix`
- **Execution permissions**: Use `chmod +x` on executables after copying to SD card
- **Binary compatibility**: Ensure you're using the correct build for your platform (Linux vs MIPS)

## 🤝 Contributing

Contributions are welcome! Please ensure:
1. Code follows the existing style
2. All tests pass
3. New features include appropriate tests
4. Documentation is updated as needed

## 📄 License

[Include license information here]

## 📞 Support

[Include support information here]
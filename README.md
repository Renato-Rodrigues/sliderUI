# sliderUI — Miyoo Mini Plus (MinUI) Carousel Launcher

sliderUI is a kid-friendly carousel / launcher for the Miyoo Mini Plus running MinUI.
It provides:

- Horizontal carousel with centered box art and orange outline on selected item.
- Precomputed reflection images (cached) to reduce runtime cost.
- Lazy loading of box art and icons (configurable).
- `.dat` XML parsing with checksum-style cache to avoid re-parsing unchanged files.
- Installer app that runs from MinUI (no terminal required) to install/update/uninstall.
- Autorun (Kids Mode) with Konami code to exit to the full MinUI.
- Parental toggle app (accessible only from MinUI) that enables/disables autorun.
- Self-contained deployment option: `make bundle` copies required `.so` files to `deploy/lib/` and `run_sliderUI.sh` sets `LD_LIBRARY_PATH` to them at runtime.
- Deploy helper scripts for the union-miyoomini-toolchain Docker image.

---

## Project layout

```

sliderUI/
├── Makefile
├── README.md
├── build/                          # build artifacts (after compile)
├── deploy/                         # created by `make bundle` + `make install-wrapper`
│   ├── lib/                        # bundled .so files (by make bundle)
│   └── run_sliderUI.sh             # runtime wrapper (by make install-wrapper)
├── config/
│   └── sliderUI.cfg
├── data/
│   └── sliderUI_games.txt          # example of file located at (/mnt/SDCARD/Roms/sliderUI_games.txt)
├── assets/
│   ├── icons/
│   │   └── SFC.png
│   └── fonts/
│       └── default.ttf
├── src/
│   ├── main.cpp
│   ├── slider.hpp
│   ├── slider.cpp
│   ├── reflection_cache.hpp
│   ├── reflection_cache.cpp
│   ├── dat_cache.h
│   ├── dat_cache.cpp
│   └── slider_installer.cpp
├── tools/
│   ├── install_sliderUI.sh
│   ├── launch_sliderUI.sh
│   └── toggle_mode.sh
│   └── toggle_sliderUI_autorun.sh
├── build_in_toolchain.sh
└── deploy_to_sd.sh

````

---

## Quick summary of important runtime paths (on device)

- Games list (user-provided): `/mnt/SDCARD/Roms/sliderUI_games.txt`
- Game ROM: `/mnt/SDCARD/Roms/<systemFolder>/<gameFile>`
- Box art: `/mnt/SDCARD/Roms/<systemFolder>/.res/<gameBasename>.png`
- System `.dat` XML: `/mnt/SDCARD/Roms/<systemFolder>/<systemName>.dat`
- Installed app directory: `/mnt/SDCARD/App/sliderUI/`
- Installer app directory: `/mnt/SDCARD/App/sliderUI_installer/`
- Reflection cache (app): `/mnt/SDCARD/App/sliderUI/cache/reflections/`
- Dat cache (app): `/mnt/SDCARD/App/sliderUI/dat_cache.txt`
- Bundled libs (if used): `/mnt/SDCARD/App/sliderUI/lib/`
- Runtime launcher wrapper: `/mnt/SDCARD/App/sliderUI/run_sliderUI.sh`

---

## Build & deploy (step-by-step)

### **Prerequisites**

1. **Windows with WSL2 (Ubuntu/Debian)**
2. **Docker Desktop** installed and running
3. **Basic tools in WSL:**
   ```bash
   sudo apt update
   sudo apt install -y git curl build-essential
   ```

### **Step 1: Setup Toolchain (One-Time)**

```bash
# Navigate to your home directory
cd ~

# Clone the toolchain
git clone https://github.com/shauninman/union-miyoomini-toolchain.git
cd union-miyoomini-toolchain

# if this PR is not merged yet, do the changes you can find here: https://github.com/shauninman/union-miyoomini-toolchain/pull/3/files

# Build the Docker image (this takes 10-30 minutes)
docker build -t union-miyoomini-toolchain:latest .

# Verify the image was created
docker images | grep union
```

**Expected output:**
```
union-miyoomini-toolchain   latest   abc123def456   500MB
```

### **Step 2: Get Your sliderUI Project**

```bash
# Still in ~/ or wherever you want your project
cd ~

# Clone your sliderUI repository
git clone https://github.com/Renato-Rodrigues/sliderUI.git
cd sliderUI

# Make scripts executable
chmod +x full_build.sh
chmod +x deploy_to_sd.sh
```

### **Step 3: Build the Project**

```bash
# Run complete build
./full_build.sh
```

### **Step 4: Bundle Libraries (Optional but Recommended)**

```bash
# Run the bundle command inside Docker
docker run --rm \
  -u "$(id -u):$(id -g)" \
  -v "$(pwd)":/app \
  -w /app \
  union-miyoomini-toolchain:latest \
  /bin/bash -c "make bundle && make install-wrapper"

# Check the deploy directory was created
ls -la deploy/
```

### **Step 5: Deploy to SD Card**

```bash
# Insert your SD card and find its mount point
# Usually: /media/$USER/XXXXX or /mnt/XXXXX
lsblk

# Deploy (replace with your actual mount point)
./deploy_to_sd.sh /media/$USER/MYSD

# Safely unmount
sync
sudo umount /media/$USER/MYSD
```

---

## **Verify Your Build**

After building, check that you have:

```bash
ls -lh build/
```

Should show:
- `sliderUI` (~200-300KB)
- `sliderUI_installer` (~150-200KB)
- `*.sha256` files

```bash
file build/sliderUI
```

Should show:
```
build/sliderUI: ELF 32-bit LSB executable, ARM, EABI5 version 1 (SYSV), dynamically linked...
```
---


---

## **Troubleshooting SDL 1.2 Build Issues**

### **Issue: "SDL.h not found"**

The toolchain might not have SDL 1.2 development headers. Check inside the Docker container:

```bash
docker run --rm -it union-miyoomini-toolchain:latest /bin/bash

# Inside container:
ls /opt/union_toolchain/sysroot/usr/include/ | grep -i sdl
ls /opt/union_toolchain/sysroot/usr/lib/arm-linux-gnueabihf/ | grep -i sdl
```

**Expected files:**
```
/usr/include/SDL/
/usr/lib/arm-linux-gnueabihf/libSDL-1.2.so
/usr/lib/arm-linux-gnueabihf/libSDL_image-1.2.so
/usr/lib/arm-linux-gnueabihf/libSDL_ttf-2.0.so
```

If these are missing, you need to update the toolchain Dockerfile to include SDL 1.2 development packages.

### **Issue: Linking errors for SDL functions**

Add `-lpthread` to LDFLAGS in Makefile (already included above).

### **Issue: "undefined reference to pthread_create"**

The order matters in linking. Update Makefile LDFLAGS:

```makefile
LDFLAGS = -L$(SYSROOT)/usr/lib/arm-linux-gnueabihf \
	-lSDL -lSDL_image -lSDL_ttf -lstdc++fs -lpthread -lm
```

---

## **Final Verification Checklist**

Before deploying to device:

- [ ] Build completes without errors
- [ ] `build/sliderUI` and `build/sliderUI_installer` exist
- [ ] Binary size is reasonable (~200-400KB each)
- [ ] `readelf` shows SDL 1.2 dependencies (not SDL2)
- [ ] `file` command shows ARM 32-bit ELF
- [ ] `deploy/` directory contains bundled libraries (optional)
- [ ] `deploy/run_sliderUI.sh` exists (if bundled)

Check binary architecture:

```bash
file build/sliderUI
```

**Expected:**
```
build/sliderUI: ELF 32-bit LSB executable, ARM, EABI5 version 1 (SYSV), dynamically linked, interpreter /lib/ld-linux-armhf.so.3, for GNU/Linux 3.2.0, not stripped
```

---

## **Testing on Device**

1. **Insert SD card into Miyoo Mini Plus**
2. **Power on and navigate to Apps**
3. **You should see:**
   - `SliderUI Installer` app
4. **Run installer and select "Install / Update SliderUI"**
5. **After install, you should see:**
   - a new app entry `Slider Mode` app
6. To enable auto-boot (Kids Mode), use the installer button or the `Enable Auto-boot` option.
7. From inside sliderUI, enter the Konami code `↑ ↑ ↓ ↓ ← → ← → B A` to return to full MinUI.
8. To toggle autorun (parental control) from MinUI only, run the `Toggle Kids Mode` app installed by the installer (not available inside sliderUI).

---

## **Common Runtime Issues**

### **Black screen on launch**

- Check that fonts exist at `/mnt/SDCARD/App/sliderUI/assets/fonts/default.ttf`
- Check logs if MinUI provides them
- Verify screen resolution is 640x480

### **No games showing**

- Check `/mnt/SDCARD/Roms/sliderUI_games.txt` exists
- Verify format: `SystemFolder (CODE);GameFileName.ext`
- Example: `SNES (SFC);Super Mario World.sfc`

### **Missing box art**

- Box art should be at: `/mnt/SDCARD/Roms/SYSTEMFOLDER/.res/GAMENAME.png`
- The `.res` folder is hidden - make sure it exists

### **Joystick not working**

- SDL 1.2 joystick support on Miyoo Mini Plus should work automatically
- Buttons: A=0, B=1, X=2, Y=3
- D-pad uses hat events

---

### Required Third-Party Header

`stb_image_write.h` is required by `reflection_cache.cpp` for PNG writing:
```bash
# Download into src/ directory before building
cd src/
wget https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h
```
Alternatively, download manually from: https://github.com/nothings/stb/blob/master/stb_image_write.h

---

## License

MIT — modify as you need.

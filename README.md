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

These are the two recommended flows:

- **A. Build inside the union-miyoomini-toolchain Docker image (recommended)**
- **B. Deploy to SD card (host)**

Both flows are fully automated by `build_in_toolchain.sh` and `deploy_to_sd.sh` (included).

### A — Build with `shauninman/union-miyoomini-toolchain` (Docker)

1. **Clone the toolchain repo (host):**

   ```bash
   cd ~
   git clone https://github.com/shauninman/union-miyoomini-toolchain.git
   cd union-miyoomini-toolchain
   ````

2. **Prepare workspace and place `sliderUI` into it:**

   ```bash
   mkdir -p workspace
   # Option 1: copy your local project
   cp -r /path/to/your/sliderUI ./workspace/sliderUI

   # Option 2: clone sliderUI repo into workspace
   # cd workspace && git clone <your-sliderUI-repo> sliderUI
   ```

3. Install / Open **Docker Desktop** (Windows)

- If you already have it installed → open it.
- Otherwise download: [https://www.docker.com/products/docker-desktop/](https://www.docker.com/products/docker-desktop/)

- Enable **WSL Integration**
  - Open **Docker Desktop → Settings → Resources → WSL Integration**
  - Check:
  ``` 
  ☑ Enable integration with my default WSL distro
  ☑ Ubuntu / Debian (whichever your WSL uses)
  ```
  - Click **Apply & Restart**

- Confirm Docker is visible inside WSL
  - In your WSL shell: 
  ```bash
  docker --version
  ```

4. Run the toolchain image locally (one time)

  - from the MinUI toolchain folder:
  ```bash
  cd ./
  docker build -t union-miyoomini-toolchain .
  ```
  - When finished, check:
  ```bash
  docker images | grep union
  ```

  - Expected output example:
  ```
  union-miyoomini-toolchain   latest   <some-id>   <size>
  ```

5. **Build SliderUI using the helper script** (or run docker manually):

   * From the `union-miyoomini-toolchain` repo root:
  
   ```bash
   cd ./workspace/sliderUI
   chmod +x build_in_toolchain.sh
   ./build_in_toolchain.sh
   ```
   (By default it uses the repo's Dockerfile if present, otherwise pulls `shauninman/union-miyoomini-toolchain`.)

   * Equivalent manual docker command (example):

     ```bash
     docker run --rm -v "$(pwd)/workspace":/root/workspace -w /root/workspace/sliderUI shauninman/union-miyoomini-toolchain:latest /bin/bash -lc "make clean; make -j\$(nproc) all"
     ```

6. **Check artifacts on host:**
   The built artifacts are on host under:

   ```
   union-miyoomini-toolchain/workspace/sliderUI/build/
   ```

   Expected files:

   * `build/sliderUI`
   * `build/sliderUI_installer`
   * `build/*.sha256`

7. **Bundle runtime libraries (optional but recommended)**
   Inside the same Docker shell or on host (after build), from project root:

   ```bash
   cd workspace/sliderUI
   make bundle
   make install-wrapper
   ```

   * `make bundle` creates `deploy/lib/` with the exact `.so` dependencies found in the `sliderUI` ELF (copied from sysroot inside the toolchain).
   * `make install-wrapper` creates `deploy/run_sliderUI.sh`.

   After these steps `deploy/` contains `run_sliderUI.sh` and `lib/`.

---

### B — Deploy to SD card (host)

1. Mount your SD card on the host. Example mount point:

   ```
   /media/$USER/MYSD
   ```

   Replace with the actual mountpoint on your machine.

2. Use the included `deploy_to_sd.sh` to copy everything (recommended). From the `union-miyoomini-toolchain` root:

   ```bash
   ./deploy_to_sd.sh /path/to/SD_MOUNT
   ```

   What gets copied:

   * `build/sliderUI_installer` → `/mnt/SDCARD/App/sliderUI_installer/sliderUI_installer`
   * `assets/`, `data/`, `config/`, `tools/` → under installer folder
   * optionally `build/sliderUI` → `/mnt/SDCARD/App/sliderUI/sliderUI`
   * if you ran `make bundle` & `make install-wrapper`, copy `deploy/*` contents into `/mnt/SDCARD/App/sliderUI/` instead to have bundled libs + `run_sliderUI.sh`.

3. **Manual copy (alternate)**:

   From the `union-miyoomini-toolchain/workspace/sliderUI` directory:

   * Copy installer app:
```bash
     # Replace /media/<user>/MYSD with your actual SD card mount point
     SD_MOUNT="/media/$USER/MYSD"
     
     # Copy installer binary and resources
     mkdir -p "$SD_MOUNT/App/sliderUI_installer"
     cp build/sliderUI_installer "$SD_MOUNT/App/sliderUI_installer/sliderUI_installer"
     chmod +x "$SD_MOUNT/App/sliderUI_installer/sliderUI_installer"
     
     # Copy installer resources (adjust paths to match your repo structure)
     cp -r assets "$SD_MOUNT/App/sliderUI_installer/" 2>/dev/null || echo "No assets dir"
     cp -r data "$SD_MOUNT/App/sliderUI_installer/" 2>/dev/null || echo "No data dir"
     cp -r config "$SD_MOUNT/App/sliderUI_installer/" 2>/dev/null || echo "No config dir"
     cp -r tools "$SD_MOUNT/App/sliderUI_installer/" 2>/dev/null || echo "No tools dir"
     
     # Create installer metadata
     cat > "$SD_MOUNT/App/sliderUI_installer/metadata.txt" <<EOF
title=SliderUI Installer
description=Install SliderUI from the MinUI menu (no terminal)
exec=/mnt/SDCARD/App/sliderUI_installer/sliderUI_installer
EOF
```

   * If you ran `make bundle` and `make install-wrapper`:
```bash
     # Copy bundled app with dependencies
     mkdir -p "$SD_MOUNT/App/sliderUI"
     cp -r deploy/* "$SD_MOUNT/App/sliderUI/"
     cp build/sliderUI "$SD_MOUNT/App/sliderUI/sliderUI"
     chmod +x "$SD_MOUNT/App/sliderUI/sliderUI"
     chmod +x "$SD_MOUNT/App/sliderUI/run_sliderUI.sh"
     
     # Create app metadata (using wrapper)
     cat > "$SD_MOUNT/App/sliderUI/metadata.txt" <<EOF
title=Slider Mode
description=Kid-friendly slider UI
exec=/mnt/SDCARD/App/sliderUI/run_sliderUI.sh
EOF
```

   * If you did NOT use bundling (direct binary only):
```bash
     # Copy main app binary only
     mkdir -p "$SD_MOUNT/App/sliderUI"
     cp build/sliderUI "$SD_MOUNT/App/sliderUI/sliderUI"
     chmod +x "$SD_MOUNT/App/sliderUI/sliderUI"
     
     # Create app metadata (direct binary)
     cat > "$SD_MOUNT/App/sliderUI/metadata.txt" <<EOF
title=Slider Mode
description=Kid-friendly slider UI
exec=/mnt/SDCARD/App/sliderUI/sliderUI
EOF
```

4. Unmount SD card safely and put it into Miyoo Mini Plus.

---

## On-device usage (MinUI)

* Boot device and open **Apps**:

  * `SliderUI Installer` — runs the installer UI (Install / Enable Auto-boot / Disable Auto-boot / Uninstall / Exit).
  * After using **Install**, a new app entry `Slider Mode` will appear (or installer created autorun).
* To enable auto-boot (Kids Mode), use the installer button or the `Enable Auto-boot` option.
* From inside sliderUI, enter the Konami code `↑ ↑ ↓ ↓ ← → ← → B A` to return to full MinUI.
* To toggle autorun (parental control) from MinUI only, run the `Toggle Kids Mode` app installed by the installer (not available inside sliderUI).

---

## Notes & troubleshooting

* If `make` complains about missing SDL headers/libs inside the docker container, ensure the `shauninman/union-miyoomini-toolchain` image was built/pulled correctly. Paste build errors here and I will debug.
* If binary crashes on device due to missing `.so`, use the `bundle` output copied to `/mnt/SDCARD/App/sliderUI/lib/` and launch via `/mnt/SDCARD/App/sliderUI/run_sliderUI.sh`.
* If you want a full tarball of the repo with all files populated, say so and I will generate the archive contents for you to copy.

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

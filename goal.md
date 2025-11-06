
# Goal:
Add support in MinUI to a pak app that can run two compiled C UIs. 
The menu.elf file includes a way to change configurations by the user.
The sliderUI.elf file includes an alternative UI to show and launch games.
When the alternative UI runs and exits the normal minUI should run normally.

# Coding constraints:
- Do not use dynamic allocation inside render loop.
- Do not use recursion.
- UI must not directly read or write files.
- Image decoding must happen at most once per frame.
- Cache must contain only {previous, current, next}.
- Sorting logic must be implemented in pure comparator functions.


SliderUI



C++ app developed to work on minUI in the myioo mini plus (https://github.com/shauninman/MinUI):

Final release files:
`/mnt/SDCARD/tools/sliderUI/menu.elf`
`/mnt/SDCARD/tools/sliderUI/sliderUI.elf`
`/mnt/SDCARD/tools/sliderUI/sliderUI_cfg.json`
`/mnt/SDCARD/tools/sliderUI/gameList.csv`
`/mnt/SDCARD/tools/sliderUI/kidsMode.sh`
`/mnt/SDCARD/tools/sliderUI/launch.sh`
`/mnt/SDCARD/tools/sliderUI/kids_mode_auto_resume.txt`
`/mnt/SDCARD/tools/sliderUI/bckg.png or .jpg or .webp`
`/mnt/SDCARD/tools/sliderUI/img/gameName.png or .jpg or .webp`
`/mnt/SDCARD/tools/sliderUI/ctrl/gameName.png or .jpg or .webp`
`/mnt/SDCARD/tools/sliderUI/logs/`


# 1. Main menu (menu.elf)

Menu UI following minimalist text based aesthetics of minUI, with only text, besides the toggle. Menu executable located at after compile: `/mnt/SDCARD/tools/sliderUI/menu.elf` 

Main menu options: `sort`, `start game`, `kids mode` and `games list` 

Controls:
Up and down goes to the next or previous right text
If you press b: You exit the slider ui menu
If you press a you select, change or enter the option

Help information
Located at the bottom right of the screen like in minUI it displays a circle with button letter or icon inside + text at the side with what the button press does.

Add lightweight logging to `/mnt/SDCARD/tools/sliderUI/logs/` with rotation and timestamps. Include level (INFO/ERROR).

## 1.1 sort

`sort` text in the left, and text in the right with one of the following options `alphabetical` or `release` or `custom` text
Controls
If you press a while the toggle is active you change the toggle right options, looping through the three options.
the current option is saved to the sliderUI config file (`/mnt/SDCARD/tools/sliderUI/sliderUI_cfg.json`).  
If you press b you exit the main menu
If you press down it makes the `start game mode` option active
If you press up, it does nothing.
Help information
In this menu it shows `A` inside a white circle and the `change` text, with a rounded gray rectangle surrounding them

## 1.2 start game mode

`start game mode` text in the left, and text in the right with one of the following options `last played` or `first game` text
Controls
If you press a while the toggle is active you change the toggle right options, looping through the two options.
the current option is saved to the sliderUI config file (`/mnt/SDCARD/tools/sliderUI/sliderUI_cfg.json`).  
If you press b you exit the main menu
If you press down it makes the `kids mode` option active
If you press up, it does nothing.
Help information
In this menu it shows `A` inside a white circle and the `change` text, with a rounded gray rectangle surrounding them

## 1.3 kids mode

`kids mode` text in the left and toggle with `enable/disable` options in the right
Controls
If you press a while the toggle is active you change the toggle between enable and disable, and vice versa 
launch a script named kidsmode.sh
If you press b you exit the main menu
If you press down, it makes the `games list` option active
If you press up, it makes the `start game mode` option active
Help information
In this menu it shows `A` inside a white circle and the `change` text, with a rounded gray rectangle surrounding them


## 1.4 game list

`game list` text in the left    
Controls
If you press a while the game list is active you go to the game list menu 
If you press b you exit the main menu
If you press down it does nothing
If you press up, it goes to the `kids mode` option.
Help information
In this menu it shows `A` inside a white circle and the `enter` text, with a rounded gray rectangle surrounding them


### 1.4.1. Games list menu 

List all games in the gamelist file `/mnt/SDCARD/tools/sliderUI/gameList.csv` using the sort method defined in the file `/mnt/SDCARD/tools/sliderUI/sliderUI_cfg.json`.
Controls: 
If you press up or down you move to the previous game or the next game
If you press b while no game is selected you go back to the main menu, with the `game list` option selected
If you press y, you remove the row for the game from the gameList.csv file, from the list showed in this menu, and it updates the list showed in the screen
Confirm deletion / removal (Y) with undo window (e.g., “Confirm with Y / Cancel with B”).
Represent confirmation with a boolean state flag in UI loop, not a nested state machine.
If you press a 
It does nothing if you are not in custom sort mode
If it is in custom sort mode: 
It selects the game (make it bold)
If you press up or down, you move the game up or down in the list. I.e. the current game gets a value +1 in the column order of the gameList.csv if you press down, and the next game gets a value -1 in the column order of the gameList.csv file, and vice versa if you press up.
If you press a or b you deselect the game and go back to the normal behavior of up and down moving to the next or previous game
Store order as an integer in gameLIst. When moving a row, swap integer values with neighbor, then normalize (reindex contiguous 0..N-1) to avoid gaps/duplicates.
If order missing, assign max_order + 1 and save immediately.
Wghne moving rows in custom mode, do: swap orders → normalize orders → write to CSV immediately
Help information
In this menu it shows `Y` inside a white circle and the `remove` text, with a rounded gray rectangle surrounding them
If a game is selected and it is custom mode, shows instead the down arrow and the text `move`, with a rounded gray rectangle surrounding them 


# 2. launch.sh

minUI launch.sh pack file that launches the menu.elf app
`/mnt/SDCARD/tools/sliderUI/launch.sh`


# 3. kidsMode.sh

location: `/mnt/SDCARD/tools/sliderUI/kidsMode.sh`
Script that:
Disable minUI auto_resume feature.
remove the current minUI `auto_resume.txt` file: rm -f "/mnt/SDCARD/.userdata/shared/.minui/auto_resume.txt"
copy the file `/mnt/SDCARD/tools/sliderUI/kids_mode_auto_resume.txt` to `/mnt/SDCARD/.userdata/shared/.minui/auto_resume.txt`, renaming it
create the file that enables simple mode: `/mnt/SDCARD/.userdata/shared/enable-simple-mode`

# 4. kids_mode_auto_resume.txt

location: `/mnt/SDCARD/tools/sliderUI/kids_mode_auto_resume.txt`
auto_resume file that:
# Launch sliderUI in kids mode
"/mnt/SDCARD/tools/sliderUI" --mode kidsmode --path gameFolder --exit konami
# remains active while browsing and entering games from sliderUI. 
Only after the konami exit code is used to exit sliderUI:
removes the auto_resume file from `/mnt/SDCARD/.userdata/shared/.minui/auto_resume.txt`
either just exits sliderUI if the minUI is active afterwards or reboot Myioo mini to run the normal minUI

# 5. sliderUI config (sliderUI_cfg.json)

JSON format using nlohmann/json
`/mnt/SDCARD/tools/sliderUI/sliderUI_cfg.json`
Default JSON schema + example
```
{
  "version": 1,
  "ui": {
    "resolution": [640, 480],
    "background": "bckg.png",
    "title": { "x": 20, "y": 20, "size": 24, "font": "default" },
    "release": { "x": 20, "y": 44, "size": 12, "font": "default" },
    "platform": { "x": 20, "y": 64, "size": 12, "font": "default" },
    "buttons": { "x": 480, "y": 440, "size": 12, "font": "default" },
    "game_image": { "x": 200, "y": 140, "width": 240, "height": 160, "margin": 8 },
    "selected_contour": { "stroke": 3, "radius": 8, "color": "#FFFFFF" }
  },
  "behavior": {
    "sort_mode": "alphabetical",   // "alphabetical" | "release" | "custom"
    "start_game": "last_played",   // "last_played" | "first_game"
    "kids_mode_enabled": false,
    "exit_mode": "default"             // "default" | "konami"
  },
  "platform": {
    "icons_path": "platform/",
    "image_formats": [ "png", "jpg", "webp" ],
    "image_max_dimensions": [640,480]
  },
  "logging": {
    "enabled": true,
    "dir": "logs/",
    "max_files": 10
  }
}
```
Keep a constexpr default object in code. On load(), for each missing key use default. Validate types (e.g., sizes are ints).
Define a constexpr in-code default object, and when loading JSON:
Only override fields that exist and match the expected type.
Never overwrite existing config unless explicitly saving.

 
# 6. Game list file (gameList.csv)

Stores game list to be used in slideUI and menu
`/mnt/SDCARD/tools/sliderUI/gameList.csv`
Format:
UTF-8, newline LF, delimiter ;
Support quoted fields (game names with semicolons). Prefer using a robust CSV parser library (libcsv for example).
```
  gamePath; order; gameName; release 
```
gamePath is mandatory and points to the path of the game
Order is mandatory and it sites the order for the custom sort
 gameName is not mandatory, but if it exists it is used in sliderUI to display the game name instead of the game name obtained from the regex that clears the game file name
Release is not mandatory, but if it exists it is used in sliderUI to order game if sort release mode is chosen.

# 7. sliderUI (sliderUI.elf)

Game browser UI following the format of the image file sliderUI. 
sliderUI executable located after compile at: `/mnt/SDCARD/tools/sliderUI/sliderUI.elf` 
The executable accept as parameters:
    --mode: for now only `kidsmode` as a option
                --path gameFolder : folder for current game
                -- exit konami : exit key combination to exit menu  
                 e.g.  "/mnt/SDCARD/tools/sliderUI" --mode kidsmode --path gameFolder --exit konami

On start:
Reads the gameList.csv file to define games to show in the menu
The current active game is defined as
If start game mode is set to `last played`, it will read the last game from the sliderUI_cfg.json file and try to find it in the gamelist. If it finds it sets the current active game to it, if not it sets to 0
If start game mode is set to `first game`, it will set the  current active game to the one with index 0

Games can be ordered alphabetically, by release or by custom order.
When it starts it loads the game list and order the list according the sort mode selected that can be read from the config file `sliderUI_cfg.json`:
If the sort mode is alphabetically, it will order the gameList alphabetically
If the sort mode is custom, it will use the information in the gamelist order column to sort the games.
If a game has no value in the order column, it will assign the last order index+1 to the game, and save this information to the file gameList.csv.
If the sort mode is release date, it will use the release data information to sort the games
Release date should either be either YYYY-MM-DD or YYYY or YYYY-MM format. Implement parser with fallback (unknown → place at end).
On sort change
The current active game to the one with index 0

Menu appearance
Background image that fills screen
`/mnt/SDCARD/tools/sliderUI/bckg.png or jpg or webp`
Resolution: 640x480
GameName text title:
Displays the current selected game name at the position, size and font defined in the title text option in the cfg file.
The current game name is the one found at the column gameName of the gameList.csv file.
If there is no name defined, the filename without extension for the game is extracted from the gamePath information of the gameList.csv file.  
Release text:
Displays the current selected game release information at the position, size and font defined in the cfg file.
The current game release information is the one found at the column release of the gameList.csv file.
If there is no release defined, it does not show the release text  
Platform icon:
Displays the current selected game platform icon at the position, and size defined in the cfg file.
The platform icons can be found at: `/mnt/SDCARD/tools/sliderUI/platform/platformName.png or .jpg or .webp`
The platformName corresponds to the last folder name of the gamePath, after removing any contents in parenthesis and and leading or trailing spaces
 Platform text:
Displays the current selected game platform text at the position, size and font defined in the cfg file.
The current game platform corresponds to the last folder name of the gamePath, after removing any contents in parenthesis and and leading or trailing spaces.
Help information
Displays the buttons information in the position, size and font defined in the cfg file.
The buttons information to be displayed are menu dependent, and should follow the aesthetics already used to display them in minUI (circle with button letter or icon inside + text at the side with what the button press does). 
Game image
Displays three game images in the screen
The active game is displayed at the positions and size defined in the game image option in the cfg file.
The previous game is displayed with a x = x - w - margin
The next game is displayed with a x = x + w + margin
Selected contour
Displays a rounded rectangle with the stroke, width, height and color defined at the selected contour option in the cfg file.  

Controls:
Left = previous (index -1), Right = next (index +1) (or explicitly document circular increment direction).
A, enters the game
X,  change the sort order and update the view
Y, remove the active game from the gameList, and move to the next game, and update the view. THis is disabled if you start the slider UI if in Kids mode.
Confirm deletion / removal (Y) with undo window (e.g., “Confirm with Y / Cancel with B”).
Represent confirmation with a boolean state flag in UI loop, not a nested state machine.
Exit Key:
If exit mode is empty or set to default, if you press b while sliderUI is active you close sliderUI
If exit mode is set to konami, you need to press the konami code to exit sliderUI
Document exact key sequence and timeout; implement tolerant input buffer and clear it properly on exit.
When in konami exit mode, always clear input buffer on successful exit sequence to avoid  inputs bleed into MinUI.
Menu, if you press the menu key it shows an image with the controls over the sliderUI menu. 
The img is shown only if it can be found at: `/mnt/SDCARD/tools/sliderUI/ctrl/gameName.png or .jpg or .webp`
If you press a or b while the image is being shown, it removes it and goes back to the sliderUI menu.
While the image is being shown you cannot use the controls defined in the sliderUI

Help information
Located at the bottom right of the screen like in minUI it displays a circle with button letter or icon inside + text at the side with what the button press does.
In the sliderUI menu it shows `A` inside a white circle and the `PLAY` text, with a rounded gray rectangle surrounding them
If we are displaying the controls image, it shows `B` inside a white circle and the `EXIT` text, with a rounded gray rectangle surrounding them

Lazy-load images (only current/prev/next), downscale large images on load, and free textures when not used.


Use an image decoding library that supports webp/jpg/png; avoid loading full-resolution images into memory if not needed.
 
Show progress or spinner when loading the sliderUI menu

Add lightweight logging to `/mnt/SDCARD/tools/sliderUI/logs/` with rotation and timestamps. Include level (INFO/ERROR).


# 8. Implementation

Key rule: UI never touches CSV or JSON directly.

Single C++ codebase with shared library for: file I/O, CSV manager, config manager, image loader, UI primitives. Build two small binaries that link same libs.

Data layer: Game object { path, order:int, name, release:date, platform }.

UI layer: decoupled renderer + input. Keep update loop simple and deterministic.
## 8.1 Layers & responsibilities example

```
core/
  config_manager.h/.cpp
  game_db.h/.cpp
  log.h/.cpp
  sort.h/.cpp
  image_loader.h/.cpp   // pure decode+resize but NO caching here
  file_utils.h/.cpp

ui/
  menu_ui.cpp
  slider_ui.cpp
  renderer.h/.cpp       // only draw text, images, rectangles
  image_cache.h/.cpp    // small LRU cache that calls image_loader
```
### 8.1.1 Core / Data Layer (core/)


ConfigManager — load/save JSON, typed getters, defaults.
GameDB — parses gameList.csv, provides vector<Game>, supports add/remove/reorder, persistence (atomic writes + flock).
ImageManager (data-facing) — file existence checks, image metadata, queueing decode requests.
Logger — non-blocking file logger with rotation.
Pure logic: sorting comparators, path -> platform extraction, release parsing, order normalization.
### 8.1.2 UI Layer (ui/)


MenuUI — menu.elf UI, very small: reads/writes config via ConfigManager, launches slider.
SliderUI — rendering loop, input handling, animation, calls GameDB and ImageManager for data.
Renderer — thin abstraction over minUI drawing calls (text, blit image, draw rounded rect). Accepts pre-decoded textures.
### 8.1.3 Glue
AppController orchestrates: load config → load GameDB → prepare initial view → enter UI loop. It calls only public interfaces (no direct file I/O from UI).
E.g. API style:
```
// core/game_db.h
class GameDB {
public:
  bool load(const std::string& csv_path);
  const std::vector<Game>& games() const;
  void move_up(size_t index);
  void move_down(size_t index);
  bool remove(size_t index); // marks removal or deletes immediately per policy
  bool commit(); // atomic write to disk
};
```
UI calls game_db.move_down(i) then game_db.commit() (or commit() delayed for batch).

# 9. Efficient image load / cache strategy
No background thread.
Decode one image per frame, max 50ms work per frame.
Cache size fixed at 3 textures.
## 9.1 Goals
Only keep decoded textures for previous/current/next.
Decode & downscale to display size (avoid full-res memory).
Avoid blocking UI loop on SD read.

## 9.2 Components
ImageMeta — path, file size, last-modified, target width/height.
Texture — GPU/renderer-ready resource (e.g., RGB565 buffer).
ImageCache — LRU with fixed capacity (default 3). Exposes get(path) → optional<Texture>.
ImageWorker — single background thread that decodes & downscales; pushes result to ImageCache and notifies UI (thread-safe flag or event queue).
## 9.3 Behavior
On selection change:
UI checks ImageCache for next/prev/current.
If missing, enqueue decode tasks for them with priority: current > next > prev.
Decoding pipeline:
Read file chunk → decode (libwebp/stb_image) → downscale to target dims → convert to GPU format → store in ImageCache.
Memory: cap decoded buffers size and evict least recently used.
Fallback: if task pending, renderer draws placeholder & spinner.
## 9.4 Implementation details (practical)
Keep ImageCache capacity = 3 by default (configurable).
Use std::mutex + condition_variable for worker queue.
Avoid fsync() per image write; only needed for config/gameList updates.
If threading is too complex for constrained environment, perform cooperative loading: decode one image per UI frame but ensure not to exceed frame budget (e.g., decode at most 1 image per 50ms).

# 10 Simple, predictable sorting logic
Keep sorting pure, stable, and isolated from UI.
# 10.1 Rules
# 10.1.1 Preprocess
Ensure every Game has order int: if missing → assign max_order + 1 and persist.
Parse release into optional<Date> using tolerant parser (YYYY-MM-DD, YYYY-MM, YYYY).
# 10.1.2 Comparators
alphabetical: compare display_name (case-insensitive, Unicode normalized).
release: entries with valid date come before unknown; compare by date descending (newest first) or ascending per preference (document).
custom: compare order ascending (lower → earlier). Use std::stable_sort to preserve relative order among equal keys.
# 10.1.3 Apply sort
Use stable sort everywhere to avoid reorder jitter:

```
std::stable_sort(games.begin(), games.end(), cmp_alpha);
```
# 10.1.4 When user changes sort mode
Recompute sorted list.
Reset active index to 0 (as specified) or find same game by path if preserving selection preferred (documented option).
Persist sort_mode in config.

# 10.1.5 Reindex after moves

When moving item up/down: swap order with neighbor, then call normalize_orders() which reassigns contiguous values 0..N-1 in current display order. Persist file atomically.















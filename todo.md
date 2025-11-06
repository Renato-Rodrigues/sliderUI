# SliderUI Implementation Guide

## Project-Wide Constraints
- Target: C++17
- External libraries:
  - nlohmann/json for JSON handling
  - stb_image.h for PNG/JPG decoding
  - (Optional) libwebp if explicitly needed
- No dynamic allocation inside rendering loop
- Image cache capacity fixed at 3
- Cooperative image loading (≤1 image per UI frame)
- Atomic writes for config and CSV using .tmp files and rename()
- No threads unless explicitly requested
- Platform-agnostic renderer (thin stub for MinUI)

## Implementation Status and Tasks

### 1. Core Layer Components (In Progress)

#### ✅ 1.1 ConfigManager
Fully implemented with:
- nlohmann/json integration
- Type-safe getters
- Atomic writes
- JSON validation
- Default configuration
- Exception-free hot path

#### 🔨 1.2 FileUtils
Partially implemented with:
- Atomic write operations
- Basic file operations
Needs:
```cpp
Request: "Enhance FileUtils with:
- Add comprehensive directory management
- Implement path manipulation utilities
- Add file permission handling
- Enhance error reporting
- Add file locking mechanisms
- Test atomic operations thoroughly"
```

#### 🔨 1.3 GameDB
Basic structure exists, needs enhancement:
```cpp
Request: "Enhance GameDB implementation with:
- Verify UTF-8 CSV parsing functionality
- Add comprehensive unit tests for CSV parsing
- Test and verify order normalization logic
- Implement robust error handling for malformed CSV
- Add file locking for concurrent access safety
- Document CSV format requirements"
```

#### 🔨 1.4 Logger
Basic implementation exists, needs verification:
```cpp
Request: "Enhance Logger implementation with:
- Test file rotation functionality
- Verify thread safety
- Add performance benchmarks
- Implement log cleanup
- Add structured logging support
- Test buffer threshold behavior"
```

#### 🔨 1.5 Sort Utilities
Basic sorting implemented, needs enhancement:
```cpp
Request: "Enhance Sort utilities with:
- Add Unicode-aware case-insensitive comparison
- Implement and test stable sorting
- Add comprehensive sort order tests
- Optimize performance for large lists
- Document sorting behavior
- Add sort validation utilities"
```

#### 🔨 1.6 ImageLoader
Basic implementation exists, needs verification:
```cpp
Request: "Enhance ImageLoader with:
- Test downscaling implementation
- Verify memory management
- Add format support tests
- Implement cooperative loading
- Add format validation
- Create fallback mechanisms"
```

### 2. UI Layer Components (Needs Implementation)

#### ❌ 2.1 Renderer
Basic stub exists, needs full implementation:
```cpp
Request: "Implement MinUI-specific Renderer with:
- Platform-specific drawing primitives
- Memory-efficient state management
- Fixed memory allocation strategy
- Clear platform abstraction
- Performance optimizations
- Testing infrastructure"
```

#### 🔨 2.2 ImageCache
Structure exists, partially implemented:
```cpp
Request: "Enhance ImageCache with:
- Verify LRU eviction policy (already implemented)
- Add pre-allocated memory pool for cache entries
- Benchmark performance under load
- Extend memory usage monitoring beyond entry count
- Add comprehensive tests for cooperative loading
- Document thread safety guarantees"
```

#### 🔨 2.3 MenuUI
Basic functionality exists, needs visual and UX enhancement:
```cpp
Request: "Enhance MenuUI implementation with:
- Apply BPreplay font from minUI
- Implement minimal UI design matching reference images
- Add proper selector highlight and spacing
- Add icon support in top right corner
- Enhance sort options with proper release order implementation
- Improve first game/last played game selection
- Add ON/OFF slider for kids mode toggle
- Implement games list with scroll indicators
- Add proper navigation controls display
- Memory-efficient state management for lists
- reference images:
  - Main menu: 'resources/slider ui menu main menu options.png'
  - Games list: 'resources/sliderUI menu games list view.png'"
```

#### ❌ 2.4 SliderUI
Basic structure exists, needs implementation:
```cpp
Request: "Implement SliderUI features with:
- Game navigation system
- Background image support
- Game information display
- Platform icons integration
- Control overlay system
- Fixed memory usage patterns
- reference image for background: 'resources/bckg.png'
- reference image for ui design: 'resources/sliderUI.svg'"
```

### 3. Required Scripts (Needs Implementation)

#### ❌ 3.1 Shell Scripts
```cpp
Request: "Create MinUI integration scripts:
- launch.sh for MinUI integration
- kidsMode.sh for kids mode
- kids_mode_auto_resume.txt template
- Error handling and logging
- Permission management
- Atomic operations"
```

### 4. System Integration (Needs Implementation)

#### ❌ 4.1 Input System
```cpp
Request: "Implement input handling with:
- Basic navigation controls
- Konami code sequence
- Game selection handling
- Sort mode changes
- Removal confirmation
- Input buffer management"
```

#### ❌ 4.2 Build System
```cpp
Request: "Setup build system with:
- Linux/WSL build support
- Miyoo Mini cross-compilation
- Dependency management
- Test compilation
- Optimization options
- Installation targets"
```

#### ❌ 4.3 Testing Framework
```cpp
Request: "Create test suite with:
- Core component unit tests
- UI integration tests
- Performance benchmarks
- Memory leak detection
- Cross-platform validation
- Test automation"
```

#### ❌ 4.4 Consolidate parameters
```cpp
Request: "Consolidate parameters
- Can the menu_config.h, menu_constants.h, and any other relevant ui parameter be consolidated in the config manager"
```

### 5. Documentation (Needs Implementation)

#### ❌ 5.1 Technical Documentation
```cpp
Request: "Create comprehensive documentation:
- API documentation
- Build instructions
- Configuration guide
- Testing guide
- Performance guidelines
- Memory management guide"
```

## Implementation Guidelines

### Memory Management
1. Pre-allocate buffers where possible
2. Use stack allocation for small objects
3. Clear memory pools between frames
4. Monitor peak memory usage
5. Use static analysis tools

### Error Handling
1. Return values over exceptions
2. Clear error states
3. Logging for debugging
4. Graceful fallbacks
5. User-friendly error messages

### Performance
1. Profile critical paths
2. Minimize file I/O
3. Efficient string handling
4. Smart caching strategies
5. Optimize memory layout

## Review Checklist

Before marking a task complete:
- [ ] Meets all project-wide constraints
- [ ] Follows memory management guidelines
- [ ] Includes proper error handling
- [ ] Has appropriate tests
- [ ] Documentation complete
- [ ] Performance requirements met

Legend:
- ✅ Fully implemented
- 🔨 Needs enhancement
- ❌ Needs implementation
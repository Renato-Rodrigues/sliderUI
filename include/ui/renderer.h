#pragma once
#ifndef SLIDERUI_UI_RENDERER_H
#define SLIDERUI_UI_RENDERER_H

#include <string>
#include <vector>
#include <cstddef>

namespace core {
struct Game;           // forward-declared from core/game_db.h
class ImageCache;      // forward-declared from core/image_cache.h
} // namespace core

namespace ui {

/**
 * Input events exposed by the renderer / platform layer.
 */
enum class Input {
  NONE,
  UP,
  DOWN,
  LEFT,
  RIGHT,
  A,
  B,
  X,
  Y,
  MENU
};

/**
 * Poll platform input. Stub declaration — implementation will map platform buttons to Input.
 * Should be non-blocking and return the latest single event (or NONE).
 */
Input poll_input();

/**
 * Thin Renderer stub.
 *
 * Responsibilities:
 *  - initialize and shutdown rendering subsystem
 *  - simple 2D drawing helpers used by SliderUI/MenuUI
 *
 * This header intentionally contains *only* declarations so the implementation can be
 * swapped between a MinUI-backed renderer and a headless / test stub.
 */
class Renderer {
public:
  Renderer() = default;
  ~Renderer() = default;

  // Initialize rendering backend. Return true on success.
  bool init();

  // Shutdown and release backend resources.
  void shutdown();

  // Clear current frame / draw buffer (prepare for drawing).
  void clear();

  // Present current frame to display (flip).
  void present();

  // Draw text at (x,y). highlight toggles a visual emphasis (e.g., inverted colors).
  void draw_text(int x, int y, const std::string &s, bool highlight = false);

  // Get text width in pixels (for layout calculations)
  int get_text_width(const std::string &s);

  // Draw a full-screen background image from disk (path).
  // Implementation may choose to asynchronously load or synchronously blit.
  void draw_background(const std::string &path);

  // Draw a 3-item horizontal game carousel view.
  // - view: contiguous slice of games to draw (caller decides which slice)
  // - active: index within `view` that is the currently selected (0..view.size()-1)
  // - cache: pointer to ImageCache for retrieving preloaded textures (may be nullptr)
  void draw_game_carousel(const std::vector<core::Game> &view, std::size_t active, core::ImageCache *cache);

  // Draw a simple overlay (centered message), used for spinners/errors.
  void draw_overlay(const std::string &msg);

  // Draw a selector highlight box with rounded corners
  void draw_selector(int x, int y, int width, int height);

  // Non-copyable / non-movable semantics (renderer manages backend handles)
  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;
  Renderer(Renderer&&) = delete;
  Renderer& operator=(Renderer&&) = delete;
};

} // namespace ui

#endif // SLIDERUI_UI_RENDERER_H

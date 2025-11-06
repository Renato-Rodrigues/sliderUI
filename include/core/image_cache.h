#pragma once
#ifndef SLIDERUI_CORE_IMAGE_CACHE_H
#define SLIDERUI_CORE_IMAGE_CACHE_H

#include "core/image_loader.h"

#include <optional>
#include <string>
#include <mutex>
#include <list>
#include <unordered_map>
#include <deque>
#include <utility>
#include <cstddef>

namespace core {

/**
 * ImageCache
 *
 * Cooperative LRU cache for Texture objects.
 */
class ImageCache {
public:
  explicit ImageCache(size_t capacity = 3);
  ~ImageCache();

  // Return a copy of texture if present. Moves entry to most-recently-used.
  std::optional<Texture> get(const std::string &path);

  // Enqueue a decode task with target size (cooperative). Thread-safe push.
  void preload_priority(const std::string &path, int target_w, int target_h);

  // Perform one pending decode task (if any). Decoding is done here; this may be slow.
  // Returns true if a task was executed (regardless of decode success).
  bool tick_one_task();

  // Current number of cached entries
  size_t size() const;

private:
  // non-copyable
  ImageCache(const ImageCache&) = delete;
  ImageCache& operator=(const ImageCache&) = delete;

  struct Pending {
    std::string path;
    int w;
    int h;
  };

  size_t capacity_;

  // LRU: front = most recent, back = least recent
  std::list<std::string> lru_list_;
  // map path -> pair<Texture, iterator into lru_list_>
  std::unordered_map<std::string, std::pair<Texture, std::list<std::string>::iterator>> cache_;

  // pending tasks queue protected by mutex (no threads created here, but safe)
  std::deque<Pending> pending_tasks_;
  mutable std::mutex pending_mutex_;
};

} // namespace core

#endif // SLIDERUI_CORE_IMAGE_CACHE_H

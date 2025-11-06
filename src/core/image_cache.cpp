#include "core/image_cache.h"
#include "core/image_loader.h"

#include <algorithm>
#include <iostream>

using core::ImageCache;
using core::Texture;

namespace core {

ImageCache::ImageCache(size_t capacity)
  : capacity_(capacity), lru_list_(), cache_(), pending_tasks_(), pending_mutex_() {
  if (capacity_ == 0) capacity_ = 3;
}

ImageCache::~ImageCache() = default;

std::optional<Texture> ImageCache::get(const std::string &path) {
  auto it = cache_.find(path);
  if (it == cache_.end()) return std::nullopt;
  // move to front (most recent)
  lru_list_.erase(it->second.second);
  lru_list_.push_front(path);
  it->second.second = lru_list_.begin();
  return std::optional<Texture>(it->second.first); // copy
}

void ImageCache::preload_priority(const std::string &path, int target_w, int target_h) {
  std::lock_guard<std::mutex> guard(pending_mutex_);
  // Enqueue at the back => FIFO processing by tick_one_task().
  pending_tasks_.push_back(Pending{path, target_w, target_h});
}

bool ImageCache::tick_one_task() {
  Pending task;
  {
    std::lock_guard<std::mutex> guard(pending_mutex_);
    if (pending_tasks_.empty()) return false;
    // pop from back to process oldest-of-priorities? We pushed new at front for priority,
    // so pop from front to get newest priority. We'll pop front.
    task = std::move(pending_tasks_.front());
    pending_tasks_.pop_front();
  }

  // If already cached, refresh LRU and skip decoding
  auto it = cache_.find(task.path);
  if (it != cache_.end()) {
    // refresh recency
    lru_list_.erase(it->second.second);
    lru_list_.push_front(task.path);
    it->second.second = lru_list_.begin();
    return true;
  }

  // decode (may be slow); decode_to_texture returns optional<Texture>
  auto opt = decode_to_texture(task.path, task.w, task.h);
  if (!opt.has_value()) {
    // failed decode: still considered a processed task
    return true;
  }

  Texture tex = std::move(*opt);

  // insert into cache, evict LRU if needed
  if (cache_.size() >= capacity_) {
    // remove least recently used (back)
    std::string oldest = lru_list_.back();
    lru_list_.pop_back();
    cache_.erase(oldest);
  }
  // insert at front
  lru_list_.push_front(task.path);
  cache_.emplace(task.path, std::make_pair(std::move(tex), lru_list_.begin()));
  return true;
}

size_t ImageCache::size() const {
  return cache_.size();
}

} // namespace core

#include "wayvault/tile_cache.h"

#include <algorithm>

#include "wayvault/checksum.h"
#include "wayvault/index.h"

namespace wayvault {

TileCache::TileCache(size_t capacity) : capacity_(capacity == 0 ? 1 : capacity) {}

TileView TileCache::view_for(const VaultFile& file, uint32_t tile_id) {
  auto existing = tiles_.find(tile_id);
  if (existing == tiles_.end()) {
    const TileRecord* record = find_tile(file, tile_id);
    if (record == nullptr) {
      throw DecodeError("tile id not found");
    }
    auto decoded = std::make_unique<DecodedTile>(decode_tile(file, *record));
    existing = tiles_.emplace(tile_id, std::move(decoded)).first;
    touch(tile_id);
    evict_if_needed();
    existing = tiles_.find(tile_id);
    if (existing == tiles_.end()) {
      throw DecodeError("tile evicted while being decoded");
    }
  } else {
    touch(tile_id);
  }

  const DecodedTile& tile = *existing->second;
  TileView view;
  view.data = tile.pixels.data();
  view.size = tile.pixels.size();
  view.tile_id = tile.id;
  view.generation = generation_;
  return view;
}

const DecodedTile* TileCache::get(uint32_t tile_id) const {
  auto it = tiles_.find(tile_id);
  return it == tiles_.end() ? nullptr : it->second.get();
}

void TileCache::clear() {
  tiles_.clear();
  lru_.clear();
  ++generation_;
}

void TileCache::touch(uint32_t tile_id) {
  lru_.erase(std::remove(lru_.begin(), lru_.end(), tile_id), lru_.end());
  lru_.push_back(tile_id);
}

void TileCache::evict_if_needed() {
  while (tiles_.size() > capacity_ && !lru_.empty()) {
    uint32_t victim = lru_.front();
    lru_.erase(lru_.begin());
    auto it = tiles_.find(victim);
    if (it != tiles_.end()) {
      tiles_.erase(it);
      ++generation_;
    }
  }
}

uint32_t sample_view_bytes(const TileView& view, uint16_t start, uint16_t count) {
  if (view.empty()) {
    return 0;
  }
  size_t pos = start % view.size;
  size_t available = view.size - pos;
  size_t take = std::min<size_t>(count == 0 ? 1 : count, available);
  return fnv1a32(view.data + pos, take);
}

}  // namespace wayvault

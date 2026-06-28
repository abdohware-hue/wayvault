#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include "wayvault/container.h"
#include "wayvault/tile_decoder.h"

namespace wayvault {

struct TileView {
  const uint8_t* data = nullptr;
  size_t size = 0;
  uint32_t tile_id = 0;
  uint32_t generation = 0;

  bool empty() const { return data == nullptr || size == 0; }
};

class TileCache {
 public:
  explicit TileCache(size_t capacity);

  TileView view_for(const VaultFile& file, uint32_t tile_id);
  const DecodedTile* get(uint32_t tile_id) const;
  void clear();
  size_t size() const { return tiles_.size(); }
  size_t capacity() const { return capacity_; }
  uint32_t generation() const { return generation_; }

 private:
  void touch(uint32_t tile_id);
  void evict_if_needed();

  size_t capacity_;
  uint32_t generation_ = 1;
  std::unordered_map<uint32_t, std::unique_ptr<DecodedTile>> tiles_;
  std::vector<uint32_t> lru_;
};

uint32_t sample_view_bytes(const TileView& view, uint16_t start, uint16_t count);

}  // namespace wayvault

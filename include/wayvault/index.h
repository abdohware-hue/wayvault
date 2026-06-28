#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "wayvault/container.h"

namespace wayvault {

struct TileIndexEntry {
  uint32_t id = 0;
  size_t tile_position = 0;
  std::string name;
  uint32_t payload_hash = 0;
};

struct VaultIndex {
  std::unordered_map<uint32_t, TileIndexEntry> by_id;
  std::unordered_map<std::string, uint32_t> by_name;
  std::vector<uint32_t> order;
};

VaultIndex build_index(const VaultFile& file);
const TileRecord* find_tile(const VaultFile& file, uint32_t tile_id);
std::string tile_display_name(const VaultFile& file, const TileRecord& tile);
std::vector<uint32_t> sorted_tile_ids(const VaultFile& file);

}  // namespace wayvault

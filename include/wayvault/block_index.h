#pragma once

#include <cstdint>
#include <vector>

#include "wayvault/container.h"

namespace wayvault {

struct BlockIndexEntry {
  uint32_t tile_id = 0;
  uint32_t offset = 0;
  uint32_t size = 0;
  uint32_t flags = 0;
};

struct BlockIndex {
  std::vector<BlockIndexEntry> entries;
};

BlockIndex parse_block_index(const VaultFile& file);
const BlockIndexEntry* find_block_entry(const BlockIndex& index, uint32_t tile_id);
uint32_t block_index_digest(const BlockIndex& index);

}  // namespace wayvault

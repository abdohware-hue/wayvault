#include "wayvault/index.h"

#include <algorithm>

#include "wayvault/checksum.h"

namespace wayvault {

namespace {

uint32_t payload_hash_for(const VaultFile& file, const TileRecord& tile) {
  const SectionInfo* data_section = find_section(file, TAG_DATA);
  if (data_section == nullptr) {
    return 0;
  }
  if (tile.data_offset > data_section->size || tile.data_size > data_section->size - tile.data_offset) {
    return 0;
  }
  const uint8_t* data = section_data(file, *data_section) + tile.data_offset;
  return fnv1a32(data, tile.data_size);
}

}  // namespace

VaultIndex build_index(const VaultFile& file) {
  VaultIndex index;
  index.order.reserve(file.tiles.size());
  for (size_t i = 0; i < file.tiles.size(); ++i) {
    const TileRecord& tile = file.tiles[i];
    TileIndexEntry entry;
    entry.id = tile.id;
    entry.tile_position = i;
    entry.name = tile_display_name(file, tile);
    entry.payload_hash = payload_hash_for(file, tile);
    index.by_id[tile.id] = entry;
    if (!entry.name.empty()) {
      index.by_name[entry.name] = tile.id;
    }
    index.order.push_back(tile.id);
  }
  std::sort(index.order.begin(), index.order.end());
  index.order.erase(std::unique(index.order.begin(), index.order.end()), index.order.end());
  return index;
}

const TileRecord* find_tile(const VaultFile& file, uint32_t tile_id) {
  auto it = std::find_if(file.tiles.begin(), file.tiles.end(), [tile_id](const TileRecord& tile) {
    return tile.id == tile_id;
  });
  return it == file.tiles.end() ? nullptr : &*it;
}

std::string tile_display_name(const VaultFile& file, const TileRecord& tile) {
  if (tile.name_index < file.strings.size()) {
    return file.strings[tile.name_index];
  }
  return {};
}

std::vector<uint32_t> sorted_tile_ids(const VaultFile& file) {
  std::vector<uint32_t> ids;
  ids.reserve(file.tiles.size());
  for (const TileRecord& tile : file.tiles) {
    ids.push_back(tile.id);
  }
  std::sort(ids.begin(), ids.end());
  ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
  return ids;
}

}  // namespace wayvault

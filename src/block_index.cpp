#include "wayvault/block_index.h"

#include <algorithm>

#include "wayvault/byte_reader.h"
#include "wayvault/checksum.h"
#include "wayvault/tags.h"

namespace wayvault {

BlockIndex parse_block_index(const VaultFile& file) {
  BlockIndex index;
  const SectionInfo* section = find_section(file, TAG_INDX);
  if (section == nullptr) {
    return index;
  }
  ByteReader reader(section_data(file, *section), section->size);
  uint16_t count = reader.remaining() >= 2 ? reader.read_u16() : 0;
  index.entries.reserve(count);
  for (uint16_t i = 0; i < count; ++i) {
    if (reader.remaining() < 16) {
      throw ParseError("truncated block index");
    }
    BlockIndexEntry entry;
    entry.tile_id = reader.read_u32();
    entry.offset = reader.read_u32();
    entry.size = reader.read_u32();
    entry.flags = reader.read_u32();
    index.entries.push_back(entry);
  }
  return index;
}

const BlockIndexEntry* find_block_entry(const BlockIndex& index, uint32_t tile_id) {
  auto it = std::find_if(index.entries.begin(), index.entries.end(), [tile_id](const BlockIndexEntry& entry) {
    return entry.tile_id == tile_id;
  });
  return it == index.entries.end() ? nullptr : &*it;
}

uint32_t block_index_digest(const BlockIndex& index) {
  uint32_t state = 0x10203040u;
  for (const BlockIndexEntry& entry : index.entries) {
    state = rotate_mix(state, entry.tile_id);
    state = rotate_mix(state, entry.offset);
    state = rotate_mix(state, entry.size);
    state = rotate_mix(state, entry.flags);
  }
  return state;
}

}  // namespace wayvault

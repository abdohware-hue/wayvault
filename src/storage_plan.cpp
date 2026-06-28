#include "wayvault/storage_plan.h"

#include <algorithm>
#include <sstream>

#include "wayvault/checksum.h"
#include "wayvault/index.h"

namespace wayvault {

uint64_t align_up_u64(uint64_t value, uint64_t alignment) {
  if (alignment <= 1) return value;
  uint64_t rem = value % alignment;
  return rem == 0 ? value : value + (alignment - rem);
}

StoragePlan compact_storage_plan(std::vector<StorageExtent> extents, uint32_t alignment) {
  std::stable_sort(extents.begin(), extents.end(), [](const StorageExtent& a, const StorageExtent& b) {
    if (a.alignment != b.alignment) return a.alignment > b.alignment;
    if (a.size != b.size) return a.size > b.size;
    return a.name < b.name;
  });
  StoragePlan plan;
  uint64_t cursor = 0;
  for (StorageExtent& extent : extents) {
    uint64_t use_alignment = std::max<uint64_t>(alignment, extent.alignment);
    uint64_t aligned = align_up_u64(cursor, use_alignment);
    plan.slack_bytes += aligned - cursor;
    extent.offset = aligned;
    cursor = aligned + extent.size;
    plan.extents.push_back(extent);
  }
  plan.total_size = cursor;
  return plan;
}

StoragePlan plan_storage_for_vault(const VaultFile& file, uint32_t alignment) {
  std::vector<StorageExtent> extents;
  extents.push_back(StorageExtent{"header", 0, 16u + file.sections.size() * 20u, 16});
  for (const SectionInfo& section : file.sections) {
    extents.push_back(StorageExtent{tag_to_string(section.tag), 0, section.size, section.flags & 1u ? 64u : 16u});
  }
  for (const TileRecord& tile : file.tiles) {
    StorageExtent extent;
    extent.name = "tile/" + std::to_string(tile.id);
    extent.size = tile.data_size;
    extent.alignment = tile.codec == 0 ? 16u : 8u;
    extents.push_back(extent);
  }
  return compact_storage_plan(std::move(extents), alignment);
}

std::string format_storage_plan(const StoragePlan& plan) {
  std::ostringstream out;
  out << "total=" << plan.total_size << " slack=" << plan.slack_bytes;
  for (const StorageExtent& extent : plan.extents) {
    out << "\n  " << extent.name << " @" << extent.offset << " size=" << extent.size << " align=" << extent.alignment;
  }
  return out.str();
}

uint32_t storage_plan_hash(const StoragePlan& plan) {
  uint32_t state = rotate_mix(static_cast<uint32_t>(plan.total_size), static_cast<uint32_t>(plan.slack_bytes));
  for (const StorageExtent& extent : plan.extents) {
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(extent.name.data()), extent.name.size()));
    state = rotate_mix(state, static_cast<uint32_t>(extent.offset));
    state = rotate_mix(state, static_cast<uint32_t>(extent.size));
  }
  return state;
}

}  // namespace wayvault

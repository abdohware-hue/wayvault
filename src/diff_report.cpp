#include "wayvault/diff_report.h"

#include <map>
#include <sstream>

#include "wayvault/checksum.h"
#include "wayvault/index.h"
#include "wayvault/metadata.h"

namespace wayvault {

namespace {

uint32_t vault_shape_hash(const VaultFile& file) {
  uint32_t state = rotate_mix(file.version, file.flags);
  for (const TileRecord& tile : file.tiles) {
    state = rotate_mix(state, tile.id);
    state = rotate_mix(state, tile.width | (tile.height << 16));
    state = rotate_mix(state, tile.codec | (tile.layer << 8));
  }
  for (const std::string& s : file.strings) {
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(s.data()), s.size()));
  }
  return state;
}

std::map<uint32_t, TileRecord> by_id(const VaultFile& file) {
  std::map<uint32_t, TileRecord> out;
  for (const TileRecord& tile : file.tiles) out[tile.id] = tile;
  return out;
}

void add_item(VaultDiff& diff, std::string path, std::string before, std::string after, int severity) {
  diff.items.push_back(DiffItem{std::move(path), std::move(before), std::move(after), severity});
}

}  // namespace

VaultDiff diff_vaults(const VaultFile& before, const VaultFile& after) {
  VaultDiff diff;
  diff.before_hash = vault_shape_hash(before);
  diff.after_hash = vault_shape_hash(after);
  if (before.version != after.version) add_item(diff, "version", std::to_string(before.version), std::to_string(after.version), 3);
  if (before.flags != after.flags) add_item(diff, "flags", std::to_string(before.flags), std::to_string(after.flags), 2);
  auto a = by_id(before);
  auto b = by_id(after);
  for (const auto& kv : a) {
    auto it = b.find(kv.first);
    if (it == b.end()) {
      add_item(diff, "tiles/" + std::to_string(kv.first), "present", "missing", 4);
      continue;
    }
    const TileRecord& left = kv.second;
    const TileRecord& right = it->second;
    if (left.width != right.width || left.height != right.height) {
      add_item(diff, "tiles/" + std::to_string(kv.first) + "/size",
               std::to_string(left.width) + "x" + std::to_string(left.height),
               std::to_string(right.width) + "x" + std::to_string(right.height), 3);
    }
    if (left.codec != right.codec) add_item(diff, "tiles/" + std::to_string(kv.first) + "/codec", std::to_string(left.codec), std::to_string(right.codec), 2);
  }
  for (const auto& kv : b) {
    if (!a.count(kv.first)) add_item(diff, "tiles/" + std::to_string(kv.first), "missing", "present", 4);
  }
  MetadataSummary bm = summarize_metadata(before);
  MetadataSummary am = summarize_metadata(after);
  for (const auto& kv : bm.values) {
    auto it = am.values.find(kv.first);
    if (it == am.values.end()) add_item(diff, "metadata/" + kv.first, kv.second, "", 2);
    else if (it->second != kv.second) add_item(diff, "metadata/" + kv.first, kv.second, it->second, 2);
  }
  for (const auto& kv : am.values) {
    if (!bm.values.count(kv.first)) add_item(diff, "metadata/" + kv.first, "", kv.second, 1);
  }
  return diff;
}

std::string format_diff(const VaultDiff& diff) {
  std::ostringstream out;
  out << "before=" << diff.before_hash << " after=" << diff.after_hash << " changes=" << diff.items.size();
  for (const DiffItem& item : diff.items) {
    out << "\n  [" << item.severity << "] " << item.path << ": " << item.before << " -> " << item.after;
  }
  return out.str();
}

uint32_t diff_hash(const VaultDiff& diff) {
  uint32_t state = rotate_mix(diff.before_hash, diff.after_hash);
  for (const DiffItem& item : diff.items) {
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(item.path.data()), item.path.size()));
    state = rotate_mix(state, static_cast<uint32_t>(item.severity));
  }
  return state;
}

std::vector<DiffItem> filter_diff_by_severity(const VaultDiff& diff, int min_severity) {
  std::vector<DiffItem> out;
  for (const DiffItem& item : diff.items) if (item.severity >= min_severity) out.push_back(item);
  return out;
}

}  // namespace wayvault

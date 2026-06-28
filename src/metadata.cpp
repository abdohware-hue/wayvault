#include "wayvault/metadata.h"

#include <sstream>

#include "wayvault/checksum.h"

namespace wayvault {

namespace {

std::string string_or_empty(const VaultFile& file, uint16_t index) {
  if (index < file.strings.size()) {
    return file.strings[index];
  }
  return {};
}

}  // namespace

MetadataSummary summarize_metadata(const VaultFile& file) {
  MetadataSummary summary;
  uint32_t hash = 2166136261u;
  for (const MetadataEntry& entry : file.metadata) {
    std::string key = string_or_empty(file, entry.key_index);
    std::string value = string_or_empty(file, entry.value_index);
    if (!key.empty()) {
      summary.values[key] = value;
    }
    summary.flags_or |= entry.flags;
    hash = rotate_mix(hash, fnv1a32(reinterpret_cast<const uint8_t*>(key.data()), key.size()));
    hash = rotate_mix(hash, fnv1a32(reinterpret_cast<const uint8_t*>(value.data()), value.size()));
    hash = rotate_mix(hash, entry.flags);
  }
  summary.stable_hash = hash;
  return summary;
}

std::vector<std::string> collect_labels(const VaultFile& file) {
  std::vector<std::string> labels;
  for (const TileRecord& tile : file.tiles) {
    if (tile.name_index < file.strings.size()) {
      labels.push_back(file.strings[tile.name_index]);
    }
  }
  for (const MetadataEntry& entry : file.metadata) {
    std::string key = string_or_empty(file, entry.key_index);
    if (!key.empty()) {
      labels.push_back(key);
    }
  }
  return labels;
}

std::string metadata_report(const VaultFile& file) {
  MetadataSummary summary = summarize_metadata(file);
  std::ostringstream out;
  out << "metadata entries=" << summary.values.size() << " flags=" << summary.flags_or
      << " hash=" << summary.stable_hash;
  for (const auto& kv : summary.values) {
    out << "\n  " << kv.first << "=" << kv.second;
  }
  return out.str();
}

}  // namespace wayvault

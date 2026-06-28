#include "wayvault/asset_bundle.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

#include "wayvault/checksum.h"

namespace wayvault {

namespace {

void align_payload(std::vector<uint8_t>& payload, uint32_t alignment) {
  if (alignment <= 1) return;
  while (payload.size() % alignment != 0) payload.push_back(0);
}

std::vector<std::string> split_bundle_line(const std::string& line) {
  std::vector<std::string> out;
  std::string cur;
  bool escaped = false;
  for (char c : line) {
    if (escaped) {
      cur.push_back(c);
      escaped = false;
    } else if (c == '\\') {
      escaped = true;
    } else if (c == '|') {
      out.push_back(cur);
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  out.push_back(cur);
  return out;
}

std::string escape_field(const std::string& text) {
  std::string out;
  for (char c : text) {
    if (c == '|' || c == '\\') out.push_back('\\');
    out.push_back(c);
  }
  return out;
}

}  // namespace

BundleArchive build_bundle(std::vector<BundleAsset> assets, uint32_t alignment) {
  BundleArchive archive;
  std::sort(assets.begin(), assets.end(), [](const BundleAsset& a, const BundleAsset& b) {
    return a.name < b.name;
  });
  std::string previous;
  for (const BundleAsset& asset : assets) {
    if (asset.name.empty()) {
      archive.warnings.push_back("skipped unnamed asset");
      continue;
    }
    if (asset.name == previous) {
      archive.warnings.push_back("duplicate asset " + asset.name);
      continue;
    }
    previous = asset.name;
    align_payload(archive.payload, alignment);
    BundleDirectoryEntry entry;
    entry.name = asset.name;
    entry.media_type = asset.media_type.empty() ? "application/octet-stream" : asset.media_type;
    entry.offset = static_cast<uint32_t>(archive.payload.size());
    entry.size = static_cast<uint32_t>(asset.bytes.size());
    entry.checksum = fnv1a32(asset.bytes);
    entry.flags = asset.flags;
    archive.payload.insert(archive.payload.end(), asset.bytes.begin(), asset.bytes.end());
    archive.directory.push_back(std::move(entry));
  }
  return archive;
}

bool extract_asset(const BundleArchive& archive, const std::string& name, BundleAsset& asset) {
  for (const BundleDirectoryEntry& entry : archive.directory) {
    if (entry.name != name) continue;
    if (entry.offset > archive.payload.size() || entry.size > archive.payload.size() - entry.offset) return false;
    asset.name = entry.name;
    asset.media_type = entry.media_type;
    asset.flags = entry.flags;
    asset.bytes.assign(archive.payload.begin() + entry.offset, archive.payload.begin() + entry.offset + entry.size);
    return fnv1a32(asset.bytes) == entry.checksum;
  }
  return false;
}

std::vector<BundleAsset> extract_all_assets(const BundleArchive& archive, size_t max_asset_size) {
  std::vector<BundleAsset> out;
  for (const BundleDirectoryEntry& entry : archive.directory) {
    if (entry.size > max_asset_size) continue;
    BundleAsset asset;
    if (extract_asset(archive, entry.name, asset)) out.push_back(std::move(asset));
  }
  return out;
}

std::string write_bundle_manifest(const BundleArchive& archive) {
  std::ostringstream out;
  for (const BundleDirectoryEntry& entry : archive.directory) {
    out << escape_field(entry.name) << '|' << escape_field(entry.media_type) << '|'
        << entry.offset << '|' << entry.size << '|' << std::hex << entry.checksum << std::dec
        << '|' << entry.flags << '\n';
  }
  for (const std::string& warning : archive.warnings) out << "# " << warning << '\n';
  return out.str();
}

BundleArchive read_bundle_manifest(const std::string& manifest, const std::vector<uint8_t>& payload) {
  BundleArchive archive;
  archive.payload = payload;
  std::istringstream input(manifest);
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty()) continue;
    if (line[0] == '#') {
      archive.warnings.push_back(line.substr(1));
      continue;
    }
    auto parts = split_bundle_line(line);
    if (parts.size() < 6) {
      archive.warnings.push_back("bad manifest row");
      continue;
    }
    BundleDirectoryEntry entry;
    entry.name = parts[0];
    entry.media_type = parts[1];
    entry.offset = static_cast<uint32_t>(std::stoul(parts[2]));
    entry.size = static_cast<uint32_t>(std::stoul(parts[3]));
    entry.checksum = static_cast<uint32_t>(std::stoul(parts[4], nullptr, 16));
    entry.flags = static_cast<uint32_t>(std::stoul(parts[5]));
    if (entry.offset > payload.size() || entry.size > payload.size() - entry.offset) {
      archive.warnings.push_back("asset extends past payload: " + entry.name);
    }
    archive.directory.push_back(std::move(entry));
  }
  return archive;
}

std::map<std::string, size_t> count_assets_by_media_type(const BundleArchive& archive) {
  std::map<std::string, size_t> counts;
  for (const BundleDirectoryEntry& entry : archive.directory) counts[entry.media_type]++;
  return counts;
}

uint32_t bundle_hash(const BundleArchive& archive) {
  uint32_t state = rotate_mix(0x8b36a129u, static_cast<uint32_t>(archive.directory.size()));
  state = rotate_mix(state, fnv1a32(archive.payload));
  for (const BundleDirectoryEntry& entry : archive.directory) {
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(entry.name.data()), entry.name.size()));
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(entry.media_type.data()), entry.media_type.size()));
    state = rotate_mix(state, entry.offset);
    state = rotate_mix(state, entry.size);
    state = rotate_mix(state, entry.checksum);
  }
  return state;
}

}  // namespace wayvault

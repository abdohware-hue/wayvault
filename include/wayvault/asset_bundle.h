#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace wayvault {

struct BundleAsset {
  std::string name;
  std::string media_type;
  std::vector<uint8_t> bytes;
  uint32_t flags = 0;
};

struct BundleDirectoryEntry {
  std::string name;
  std::string media_type;
  uint32_t offset = 0;
  uint32_t size = 0;
  uint32_t checksum = 0;
  uint32_t flags = 0;
};

struct BundleArchive {
  std::vector<BundleDirectoryEntry> directory;
  std::vector<uint8_t> payload;
  std::vector<std::string> warnings;
};

BundleArchive build_bundle(std::vector<BundleAsset> assets, uint32_t alignment = 8);
bool extract_asset(const BundleArchive& archive, const std::string& name, BundleAsset& asset);
std::vector<BundleAsset> extract_all_assets(const BundleArchive& archive, size_t max_asset_size = 16 * 1024 * 1024);
std::string write_bundle_manifest(const BundleArchive& archive);
BundleArchive read_bundle_manifest(const std::string& manifest, const std::vector<uint8_t>& payload);
std::map<std::string, size_t> count_assets_by_media_type(const BundleArchive& archive);
uint32_t bundle_hash(const BundleArchive& archive);

}  // namespace wayvault

#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "wayvault/error.h"
#include "wayvault/tags.h"

namespace wayvault {

struct SectionInfo {
  uint32_t tag = 0;
  uint32_t offset = 0;
  uint32_t size = 0;
  uint32_t count_hint = 0;
  uint32_t flags = 0;
};

struct TileRecord {
  uint32_t id = 0;
  uint16_t name_index = 0;
  uint16_t width = 0;
  uint16_t height = 0;
  uint8_t codec = 0;
  uint8_t layer = 0;
  uint16_t flags = 0;
  uint32_t data_offset = 0;
  uint32_t data_size = 0;
  uint32_t aux = 0;
};

struct MetadataEntry {
  uint16_t key_index = 0;
  uint16_t value_index = 0;
  uint32_t flags = 0;
};

struct VaultFile {
  uint16_t version = 0;
  uint16_t flags = 0;
  std::vector<uint8_t> bytes;
  std::vector<SectionInfo> sections;
  std::vector<std::string> strings;
  std::vector<TileRecord> tiles;
  std::vector<MetadataEntry> metadata;
};

VaultFile parse_vault(const uint8_t* data, size_t size);
VaultFile parse_vault(const std::vector<uint8_t>& bytes);

const SectionInfo* find_section(const VaultFile& file, uint32_t tag);
std::vector<const SectionInfo*> find_sections(const VaultFile& file, uint32_t tag);
const uint8_t* section_data(const VaultFile& file, const SectionInfo& section);
std::vector<uint8_t> section_bytes(const VaultFile& file, uint32_t tag);
std::string describe_vault(const VaultFile& file);

}  // namespace wayvault

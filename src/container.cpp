#include "wayvault/container.h"

#include <algorithm>
#include <sstream>

#include "wayvault/byte_reader.h"

namespace wayvault {
namespace {

constexpr size_t kHeaderSize = 16;
constexpr size_t kSectionEntrySize = 20;

void parse_strings(VaultFile& file, const SectionInfo& section) {
  ByteReader reader(section_data(file, section), section.size);
  uint16_t count = reader.remaining() >= 2 ? reader.read_u16() : 0;
  file.strings.clear();
  file.strings.reserve(count);
  for (uint16_t i = 0; i < count; ++i) {
    if (reader.remaining() < 2) {
      throw ParseError("truncated string table length");
    }
    uint16_t len = reader.read_u16();
    if (len > reader.remaining()) {
      throw ParseError("string table entry exceeds section");
    }
    file.strings.push_back(reader.read_string(len));
  }
}

void parse_tiles(VaultFile& file, const SectionInfo& section) {
  ByteReader reader(section_data(file, section), section.size);
  uint16_t count = reader.remaining() >= 2 ? reader.read_u16() : 0;
  file.tiles.clear();
  file.tiles.reserve(count);
  for (uint16_t i = 0; i < count; ++i) {
    if (reader.remaining() < 24) {
      throw ParseError("truncated tile record");
    }
    TileRecord tile;
    tile.id = reader.read_u32();
    tile.name_index = reader.read_u16();
    tile.width = reader.read_u16();
    tile.height = reader.read_u16();
    tile.codec = reader.read_u8();
    tile.layer = reader.read_u8();
    tile.flags = reader.read_u16();
    tile.data_offset = reader.read_u32();
    tile.data_size = reader.read_u32();
    tile.aux = reader.read_u32();
    file.tiles.push_back(tile);
  }
}

void parse_metadata(VaultFile& file, const SectionInfo& section) {
  ByteReader reader(section_data(file, section), section.size);
  uint16_t count = reader.remaining() >= 2 ? reader.read_u16() : 0;
  file.metadata.clear();
  file.metadata.reserve(count);
  for (uint16_t i = 0; i < count; ++i) {
    if (reader.remaining() < 8) {
      throw ParseError("truncated metadata entry");
    }
    MetadataEntry entry;
    entry.key_index = reader.read_u16();
    entry.value_index = reader.read_u16();
    entry.flags = reader.read_u32();
    file.metadata.push_back(entry);
  }
}

}  // namespace

VaultFile parse_vault(const uint8_t* data, size_t size) {
  if (data == nullptr && size != 0) {
    throw ParseError("null input pointer");
  }
  ByteReader reader(data, size);
  if (size < kHeaderSize) {
    throw ParseError("input too small for WayVault header");
  }

  std::string magic = reader.read_string(4);
  if (magic != "WVLT") {
    throw ParseError("bad WayVault magic");
  }

  VaultFile file;
  file.bytes.assign(data, data + size);
  file.version = reader.read_u16();
  file.flags = reader.read_u16();
  uint16_t section_count = reader.read_u16();
  (void)reader.read_u16();
  uint32_t table_offset = reader.read_u32();

  if (file.version == 0 || file.version > 3) {
    throw ParseError("unsupported WayVault version");
  }

  size_t table_bytes = 0;
  if (!checked_mul(static_cast<size_t>(section_count), kSectionEntrySize, &table_bytes)) {
    throw ParseError("section table size overflow");
  }
  size_t table_end = 0;
  if (!checked_add(static_cast<size_t>(table_offset), table_bytes, &table_end) || table_end > size) {
    throw ParseError("section table outside input");
  }

  reader.seek(table_offset);
  file.sections.reserve(section_count);
  for (uint16_t i = 0; i < section_count; ++i) {
    SectionInfo section;
    section.tag = reader.read_u32();
    section.offset = reader.read_u32();
    section.size = reader.read_u32();
    section.count_hint = reader.read_u32();
    section.flags = reader.read_u32();

    size_t end = 0;
    if (!checked_add(static_cast<size_t>(section.offset), static_cast<size_t>(section.size), &end) || end > size) {
      throw ParseError("section outside input");
    }
    file.sections.push_back(section);
  }

  if (const SectionInfo* strings = find_section(file, TAG_STRS)) {
    parse_strings(file, *strings);
  }
  if (const SectionInfo* tiles = find_section(file, TAG_TILE)) {
    parse_tiles(file, *tiles);
  }
  if (const SectionInfo* meta = find_section(file, TAG_META)) {
    parse_metadata(file, *meta);
  }

  return file;
}

VaultFile parse_vault(const std::vector<uint8_t>& bytes) {
  return parse_vault(bytes.data(), bytes.size());
}

const SectionInfo* find_section(const VaultFile& file, uint32_t tag) {
  auto it = std::find_if(file.sections.begin(), file.sections.end(), [tag](const SectionInfo& section) {
    return section.tag == tag;
  });
  return it == file.sections.end() ? nullptr : &*it;
}

std::vector<const SectionInfo*> find_sections(const VaultFile& file, uint32_t tag) {
  std::vector<const SectionInfo*> out;
  for (const SectionInfo& section : file.sections) {
    if (section.tag == tag) {
      out.push_back(&section);
    }
  }
  return out;
}

const uint8_t* section_data(const VaultFile& file, const SectionInfo& section) {
  size_t end = 0;
  if (!checked_add(static_cast<size_t>(section.offset), static_cast<size_t>(section.size), &end) || end > file.bytes.size()) {
    throw ParseError("section slice outside owned bytes");
  }
  return file.bytes.data() + section.offset;
}

std::vector<uint8_t> section_bytes(const VaultFile& file, uint32_t tag) {
  const SectionInfo* section = find_section(file, tag);
  if (section == nullptr) {
    return {};
  }
  const uint8_t* ptr = section_data(file, *section);
  return std::vector<uint8_t>(ptr, ptr + section->size);
}

std::string describe_vault(const VaultFile& file) {
  std::ostringstream out;
  out << "WayVault v" << file.version << " sections=" << file.sections.size()
      << " strings=" << file.strings.size() << " tiles=" << file.tiles.size();
  for (const SectionInfo& section : file.sections) {
    out << "\n  " << tag_to_string(section.tag) << " offset=" << section.offset
        << " size=" << section.size << " hint=" << section.count_hint;
  }
  return out.str();
}

}  // namespace wayvault

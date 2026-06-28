#include "wayvault/sample_builder.h"

#include <algorithm>
#include <map>

#include "wayvault/tags.h"

namespace wayvault {
namespace {

void put_u8(std::vector<unsigned char>& out, uint8_t v) {
  out.push_back(v);
}

void put_u16(std::vector<unsigned char>& out, uint16_t v) {
  out.push_back(static_cast<unsigned char>(v & 0xff));
  out.push_back(static_cast<unsigned char>((v >> 8) & 0xff));
}

void put_u32(std::vector<unsigned char>& out, uint32_t v) {
  out.push_back(static_cast<unsigned char>(v & 0xff));
  out.push_back(static_cast<unsigned char>((v >> 8) & 0xff));
  out.push_back(static_cast<unsigned char>((v >> 16) & 0xff));
  out.push_back(static_cast<unsigned char>((v >> 24) & 0xff));
}

void patch_u32(std::vector<unsigned char>& out, size_t pos, uint32_t v) {
  out[pos + 0] = static_cast<unsigned char>(v & 0xff);
  out[pos + 1] = static_cast<unsigned char>((v >> 8) & 0xff);
  out[pos + 2] = static_cast<unsigned char>((v >> 16) & 0xff);
  out[pos + 3] = static_cast<unsigned char>((v >> 24) & 0xff);
}

struct SectionBuild {
  uint32_t tag = 0;
  uint32_t count_hint = 0;
  uint32_t flags = 0;
  std::vector<unsigned char> bytes;
};

}  // namespace

std::vector<unsigned char> build_sample_vault(const std::vector<SampleTileSpec>& tiles,
                                              const std::vector<SampleCommandSpec>& commands,
                                              bool include_metadata) {
  std::vector<std::string> strings;
  strings.push_back("author");
  strings.push_back("wayvault-sample");
  std::map<std::string, uint16_t> string_index;
  for (uint16_t i = 0; i < strings.size(); ++i) {
    string_index[strings[i]] = i;
  }
  for (const SampleTileSpec& tile : tiles) {
    if (!tile.name.empty() && string_index.find(tile.name) == string_index.end()) {
      string_index[tile.name] = static_cast<uint16_t>(strings.size());
      strings.push_back(tile.name);
    }
  }

  SectionBuild strs;
  strs.tag = TAG_STRS;
  strs.count_hint = static_cast<uint32_t>(strings.size());
  put_u16(strs.bytes, static_cast<uint16_t>(strings.size()));
  for (const std::string& s : strings) {
    put_u16(strs.bytes, static_cast<uint16_t>(s.size()));
    strs.bytes.insert(strs.bytes.end(), s.begin(), s.end());
  }

  SectionBuild data;
  data.tag = TAG_DATA;
  data.count_hint = static_cast<uint32_t>(tiles.size());

  SectionBuild tile_section;
  tile_section.tag = TAG_TILE;
  tile_section.count_hint = static_cast<uint32_t>(tiles.size());
  put_u16(tile_section.bytes, static_cast<uint16_t>(tiles.size()));
  for (const SampleTileSpec& tile : tiles) {
    uint32_t offset = static_cast<uint32_t>(data.bytes.size());
    data.bytes.insert(data.bytes.end(), tile.payload.begin(), tile.payload.end());
    uint16_t name = tile.name.empty() ? 0 : string_index[tile.name];
    put_u32(tile_section.bytes, tile.id);
    put_u16(tile_section.bytes, name);
    put_u16(tile_section.bytes, tile.width);
    put_u16(tile_section.bytes, tile.height);
    put_u8(tile_section.bytes, tile.codec);
    put_u8(tile_section.bytes, 0);
    put_u16(tile_section.bytes, 0);
    put_u32(tile_section.bytes, offset);
    put_u32(tile_section.bytes, static_cast<uint32_t>(tile.payload.size()));
    put_u32(tile_section.bytes, 0);
  }

  SectionBuild meta;
  meta.tag = TAG_META;
  meta.count_hint = include_metadata ? 1 : 0;
  if (include_metadata) {
    put_u16(meta.bytes, 1);
    put_u16(meta.bytes, 0);
    put_u16(meta.bytes, 1);
    put_u32(meta.bytes, 7);
  }

  SectionBuild cmds;
  cmds.tag = TAG_CMDS;
  cmds.count_hint = static_cast<uint32_t>(commands.size());
  if (!commands.empty()) {
    put_u16(cmds.bytes, static_cast<uint16_t>(commands.size()));
    for (const SampleCommandSpec& command : commands) {
      put_u8(cmds.bytes, command.op);
      put_u32(cmds.bytes, command.tile_id);
      put_u16(cmds.bytes, command.arg);
      put_u8(cmds.bytes, command.repeat);
    }
  }

  std::vector<SectionBuild> sections;
  sections.push_back(strs);
  sections.push_back(tile_section);
  sections.push_back(data);
  if (include_metadata) {
    sections.push_back(meta);
  }
  if (!commands.empty()) {
    sections.push_back(cmds);
  }

  std::vector<unsigned char> out;
  out.insert(out.end(), {'W', 'V', 'L', 'T'});
  put_u16(out, 1);
  put_u16(out, 0);
  put_u16(out, static_cast<uint16_t>(sections.size()));
  put_u16(out, 0);
  put_u32(out, 16);

  size_t table_start = out.size();
  for (size_t i = 0; i < sections.size(); ++i) {
    put_u32(out, sections[i].tag);
    put_u32(out, 0);
    put_u32(out, static_cast<uint32_t>(sections[i].bytes.size()));
    put_u32(out, sections[i].count_hint);
    put_u32(out, sections[i].flags);
  }

  for (size_t i = 0; i < sections.size(); ++i) {
    uint32_t offset = static_cast<uint32_t>(out.size());
    patch_u32(out, table_start + i * 20 + 4, offset);
    out.insert(out.end(), sections[i].bytes.begin(), sections[i].bytes.end());
  }

  return out;
}

std::vector<unsigned char> build_default_sample() {
  std::vector<SampleTileSpec> tiles;
  for (uint32_t id = 1; id <= 3; ++id) {
    SampleTileSpec tile;
    tile.id = id;
    tile.name = "tile-" + std::to_string(id);
    tile.width = 4;
    tile.height = 4;
    tile.codec = 0;
    tile.payload.resize(16);
    for (size_t i = 0; i < tile.payload.size(); ++i) {
      tile.payload[i] = static_cast<uint8_t>(id * 20 + i);
    }
    tiles.push_back(tile);
  }
  std::vector<SampleCommandSpec> commands = {
      {1, 1, 0, 2},
      {2, 2, 1, 4},
      {3, 0, 2, 4},
      {4, 1, 0, 2},
  };
  return build_sample_vault(tiles, commands, true);
}

}  // namespace wayvault

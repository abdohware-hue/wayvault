#include "wayvault/tile_decoder.h"

#include <algorithm>

#include "wayvault/byte_reader.h"
#include "wayvault/checksum.h"
#include "wayvault/index.h"

namespace wayvault {

namespace {

std::vector<uint8_t> tile_payload(const VaultFile& file, const TileRecord& tile) {
  const SectionInfo* data_section = find_section(file, TAG_DATA);
  if (data_section == nullptr) {
    throw DecodeError("tile payload requested without DATA section");
  }
  if (tile.data_offset > data_section->size || tile.data_size > data_section->size - tile.data_offset) {
    throw DecodeError("tile payload outside DATA section");
  }
  const uint8_t* base = section_data(file, *data_section) + tile.data_offset;
  return std::vector<uint8_t>(base, base + tile.data_size);
}

std::vector<uint8_t> decode_raw(const TileRecord& tile, const std::vector<uint8_t>& payload) {
  size_t expected = expected_pixel_count(tile);
  if (payload.size() < expected) {
    throw DecodeError("raw tile payload is truncated");
  }
  return std::vector<uint8_t>(payload.begin(), payload.begin() + static_cast<std::ptrdiff_t>(expected));
}

std::vector<uint8_t> decode_rle(const TileRecord& tile, const std::vector<uint8_t>& payload) {
  size_t expected = expected_pixel_count(tile);
  std::vector<uint8_t> out;
  out.reserve(expected);
  ByteReader reader(payload.data(), payload.size());
  while (!reader.empty() && out.size() < expected) {
    if (reader.remaining() < 2) {
      throw DecodeError("rle pair truncated");
    }
    uint8_t run = reader.read_u8();
    uint8_t value = reader.read_u8();
    if (run == 0) {
      continue;
    }
    size_t next_size = out.size() + run;
    if (next_size > expected) {
      throw DecodeError("rle run exceeds tile dimensions");
    }
    out.insert(out.end(), run, value);
  }
  if (out.size() != expected) {
    throw DecodeError("rle tile did not fill dimensions");
  }
  return out;
}

std::vector<uint8_t> decode_delta(const TileRecord& tile, const std::vector<uint8_t>& payload) {
  size_t expected = expected_pixel_count(tile);
  if (payload.size() < expected) {
    throw DecodeError("delta tile payload is truncated");
  }
  std::vector<uint8_t> out;
  out.reserve(expected);
  uint8_t prev = static_cast<uint8_t>(tile.aux & 0xffu);
  for (size_t i = 0; i < expected; ++i) {
    prev = static_cast<uint8_t>(prev + payload[i]);
    out.push_back(prev);
  }
  return out;
}

std::vector<uint8_t> decode_palette(const TileRecord& tile, const std::vector<uint8_t>& payload) {
  size_t expected = expected_pixel_count(tile);
  ByteReader reader(payload.data(), payload.size());
  if (reader.remaining() < 1) {
    throw DecodeError("palette header is missing");
  }
  uint8_t count = reader.read_u8();
  if (count == 0 || reader.remaining() < count) {
    throw DecodeError("palette table truncated");
  }
  std::vector<uint8_t> palette = reader.read_bytes(count);
  if (reader.remaining() < expected) {
    throw DecodeError("palette indices truncated");
  }
  std::vector<uint8_t> out;
  out.reserve(expected);
  for (size_t i = 0; i < expected; ++i) {
    uint8_t idx = reader.read_u8();
    if (idx >= palette.size()) {
      throw DecodeError("palette index outside palette");
    }
    out.push_back(palette[idx]);
  }
  return out;
}

}  // namespace

size_t expected_pixel_count(const TileRecord& tile) {
  if (tile.width == 0 || tile.height == 0) {
    throw DecodeError("zero-sized tile");
  }
  size_t expected = 0;
  if (!checked_mul(static_cast<size_t>(tile.width), static_cast<size_t>(tile.height), &expected)) {
    throw DecodeError("tile dimension overflow");
  }
  if (expected > 1u << 20) {
    throw DecodeError("tile is too large for preview decode");
  }
  return expected;
}

DecodedTile decode_tile(const VaultFile& file, const TileRecord& tile) {
  DecodedTile decoded;
  decoded.id = tile.id;
  decoded.width = tile.width;
  decoded.height = tile.height;
  decoded.codec = tile.codec;
  decoded.name = tile_display_name(file, tile);

  std::vector<uint8_t> payload = tile_payload(file, tile);
  switch (tile.codec) {
    case 0:
      decoded.pixels = decode_raw(tile, payload);
      break;
    case 1:
      decoded.pixels = decode_rle(tile, payload);
      break;
    case 2:
      decoded.pixels = decode_delta(tile, payload);
      break;
    case 3:
      decoded.pixels = decode_palette(tile, payload);
      break;
    default:
      throw DecodeError("unknown tile codec");
  }
  return decoded;
}

std::vector<DecodedTile> decode_all_tiles(const VaultFile& file, size_t limit) {
  std::vector<DecodedTile> out;
  size_t count = std::min(limit, file.tiles.size());
  out.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    out.push_back(decode_tile(file, file.tiles[i]));
  }
  return out;
}

uint32_t summarize_tile(const DecodedTile& tile) {
  uint32_t state = fnv1a32(tile.pixels);
  state = rotate_mix(state, tile.id);
  state = rotate_mix(state, static_cast<uint32_t>(tile.width) << 16 | tile.height);
  state = rotate_mix(state, tile.codec);
  return state;
}

}  // namespace wayvault

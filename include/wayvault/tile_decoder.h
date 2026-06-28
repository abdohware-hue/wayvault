#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "wayvault/container.h"

namespace wayvault {

struct DecodedTile {
  uint32_t id = 0;
  uint16_t width = 0;
  uint16_t height = 0;
  uint8_t codec = 0;
  std::string name;
  std::vector<uint8_t> pixels;
};

size_t expected_pixel_count(const TileRecord& tile);
DecodedTile decode_tile(const VaultFile& file, const TileRecord& tile);
std::vector<DecodedTile> decode_all_tiles(const VaultFile& file, size_t limit = 64);
uint32_t summarize_tile(const DecodedTile& tile);

}  // namespace wayvault

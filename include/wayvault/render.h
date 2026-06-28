#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "wayvault/tile_decoder.h"

namespace wayvault {

std::vector<uint8_t> downsample_2x(const DecodedTile& tile);
std::string ascii_preview(const DecodedTile& tile, size_t max_width = 32, size_t max_height = 16);
uint32_t compare_tiles(const DecodedTile& left, const DecodedTile& right);

}  // namespace wayvault

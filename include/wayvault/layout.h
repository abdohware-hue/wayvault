#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "wayvault/container.h"

namespace wayvault {

struct Rect {
  int32_t x = 0;
  int32_t y = 0;
  int32_t w = 0;
  int32_t h = 0;
};

struct TilePlacement {
  uint32_t tile_id = 0;
  uint8_t layer = 0;
  Rect rect;
};

std::vector<TilePlacement> build_grid_layout(const VaultFile& file, uint16_t atlas_width);
bool rectangles_overlap(const Rect& a, const Rect& b);
std::vector<std::pair<uint32_t, uint32_t>> find_layout_overlaps(const std::vector<TilePlacement>& placements);
std::string layout_report(const VaultFile& file);

}  // namespace wayvault

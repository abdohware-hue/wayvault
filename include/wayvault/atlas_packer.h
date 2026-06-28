#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace wayvault {

struct RectI {
  int x = 0;
  int y = 0;
  int w = 0;
  int h = 0;
};

struct AtlasItem {
  std::string id;
  int width = 0;
  int height = 0;
  int padding = 0;
};

struct AtlasPlacement {
  std::string id;
  int page = 0;
  RectI rect;
  bool placed = false;
};

struct AtlasPageSummary {
  int page = 0;
  int width = 0;
  int height = 0;
  int used_area = 0;
  double fill_ratio = 0.0;
};

std::vector<AtlasPlacement> pack_atlas_pages(const std::vector<AtlasItem>& items, int page_width, int page_height);
std::vector<AtlasPageSummary> summarize_atlas(const std::vector<AtlasPlacement>& placements, int page_width, int page_height);
bool rects_overlap(RectI a, RectI b);
uint32_t atlas_hash(const std::vector<AtlasPlacement>& placements);
std::string format_atlas(const std::vector<AtlasPlacement>& placements);

}  // namespace wayvault

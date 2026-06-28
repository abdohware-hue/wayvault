#include "wayvault/layout.h"

#include <sstream>

namespace wayvault {

std::vector<TilePlacement> build_grid_layout(const VaultFile& file, uint16_t atlas_width) {
  std::vector<TilePlacement> placements;
  int32_t cursor_x = 0;
  int32_t cursor_y = 0;
  int32_t row_h = 0;
  int32_t max_w = atlas_width == 0 ? 64 : atlas_width;
  for (const TileRecord& tile : file.tiles) {
    int32_t w = tile.width == 0 ? 1 : tile.width;
    int32_t h = tile.height == 0 ? 1 : tile.height;
    if (cursor_x > 0 && cursor_x + w > max_w) {
      cursor_x = 0;
      cursor_y += row_h;
      row_h = 0;
    }
    placements.push_back(TilePlacement{tile.id, tile.layer, Rect{cursor_x, cursor_y, w, h}});
    cursor_x += w;
    if (h > row_h) {
      row_h = h;
    }
  }
  return placements;
}

bool rectangles_overlap(const Rect& a, const Rect& b) {
  if (a.w <= 0 || a.h <= 0 || b.w <= 0 || b.h <= 0) {
    return false;
  }
  return a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y;
}

std::vector<std::pair<uint32_t, uint32_t>> find_layout_overlaps(const std::vector<TilePlacement>& placements) {
  std::vector<std::pair<uint32_t, uint32_t>> overlaps;
  for (size_t i = 0; i < placements.size(); ++i) {
    for (size_t j = i + 1; j < placements.size(); ++j) {
      if (placements[i].layer == placements[j].layer && rectangles_overlap(placements[i].rect, placements[j].rect)) {
        overlaps.emplace_back(placements[i].tile_id, placements[j].tile_id);
      }
    }
  }
  return overlaps;
}

std::string layout_report(const VaultFile& file) {
  auto placements = build_grid_layout(file, 64);
  auto overlaps = find_layout_overlaps(placements);
  std::ostringstream out;
  out << "placements=" << placements.size() << " overlaps=" << overlaps.size();
  for (const TilePlacement& placement : placements) {
    out << "\n  tile " << placement.tile_id << " @" << placement.rect.x << "," << placement.rect.y
        << " " << placement.rect.w << "x" << placement.rect.h << " layer=" << static_cast<int>(placement.layer);
  }
  return out.str();
}

}  // namespace wayvault

#include "wayvault/atlas_packer.h"

#include <algorithm>
#include <map>
#include <sstream>

#include "wayvault/checksum.h"

namespace wayvault {

namespace {

struct Shelf {
  int y = 0;
  int height = 0;
  int cursor = 0;
};

bool place_on_page(const AtlasItem& item, int page, int page_width, int page_height,
                   std::vector<Shelf>& shelves, AtlasPlacement& placement) {
  int w = item.width + item.padding * 2;
  int h = item.height + item.padding * 2;
  if (w > page_width || h > page_height) return false;
  for (Shelf& shelf : shelves) {
    if (h <= shelf.height && shelf.cursor + w <= page_width) {
      placement.id = item.id;
      placement.page = page;
      placement.rect = RectI{shelf.cursor + item.padding, shelf.y + item.padding, item.width, item.height};
      placement.placed = true;
      shelf.cursor += w;
      return true;
    }
  }
  int y = shelves.empty() ? 0 : shelves.back().y + shelves.back().height;
  if (y + h > page_height) return false;
  shelves.push_back(Shelf{y, h, w});
  placement.id = item.id;
  placement.page = page;
  placement.rect = RectI{item.padding, y + item.padding, item.width, item.height};
  placement.placed = true;
  return true;
}

}  // namespace

bool rects_overlap(RectI a, RectI b) {
  if (a.x + a.w <= b.x || b.x + b.w <= a.x) return false;
  if (a.y + a.h <= b.y || b.y + b.h <= a.y) return false;
  return true;
}

std::vector<AtlasPlacement> pack_atlas_pages(const std::vector<AtlasItem>& items, int page_width, int page_height) {
  std::vector<AtlasItem> sorted = items;
  std::stable_sort(sorted.begin(), sorted.end(), [](const AtlasItem& a, const AtlasItem& b) {
    int aa = a.width * a.height;
    int bb = b.width * b.height;
    if (aa != bb) return aa > bb;
    return a.id < b.id;
  });
  std::vector<std::vector<Shelf>> pages;
  std::vector<AtlasPlacement> placements;
  for (const AtlasItem& item : sorted) {
    AtlasPlacement placement;
    for (size_t page = 0; page < pages.size(); ++page) {
      if (place_on_page(item, static_cast<int>(page), page_width, page_height, pages[page], placement)) break;
    }
    if (!placement.placed) {
      pages.emplace_back();
      place_on_page(item, static_cast<int>(pages.size() - 1), page_width, page_height, pages.back(), placement);
    }
    if (!placement.placed) {
      placement.id = item.id;
      placement.page = -1;
      placement.rect = RectI{0, 0, item.width, item.height};
    }
    placements.push_back(placement);
  }
  std::stable_sort(placements.begin(), placements.end(), [](const AtlasPlacement& a, const AtlasPlacement& b) {
    return a.id < b.id;
  });
  return placements;
}

std::vector<AtlasPageSummary> summarize_atlas(const std::vector<AtlasPlacement>& placements, int page_width, int page_height) {
  std::map<int, AtlasPageSummary> pages;
  for (const AtlasPlacement& placement : placements) {
    if (!placement.placed || placement.page < 0) continue;
    AtlasPageSummary& page = pages[placement.page];
    page.page = placement.page;
    page.width = page_width;
    page.height = page_height;
    page.used_area += placement.rect.w * placement.rect.h;
  }
  std::vector<AtlasPageSummary> out;
  for (auto& kv : pages) {
    kv.second.fill_ratio = page_width > 0 && page_height > 0
        ? static_cast<double>(kv.second.used_area) / static_cast<double>(page_width * page_height)
        : 0.0;
    out.push_back(kv.second);
  }
  return out;
}

uint32_t atlas_hash(const std::vector<AtlasPlacement>& placements) {
  uint32_t state = 2166136261u;
  for (const AtlasPlacement& p : placements) {
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(p.id.data()), p.id.size()));
    state = rotate_mix(state, static_cast<uint32_t>(p.page));
    state = rotate_mix(state, static_cast<uint32_t>(p.rect.x ^ (p.rect.y << 8) ^ (p.rect.w << 16) ^ (p.rect.h << 24)));
  }
  return state;
}

std::string format_atlas(const std::vector<AtlasPlacement>& placements) {
  std::ostringstream out;
  for (const AtlasPlacement& p : placements) {
    out << p.id << " page=" << p.page << " rect=" << p.rect.x << "," << p.rect.y
        << "," << p.rect.w << "x" << p.rect.h << " placed=" << (p.placed ? "yes" : "no") << "\n";
  }
  return out.str();
}

}  // namespace wayvault

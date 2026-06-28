#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace wayvault {

struct LegendItem {
  std::string layer;
  std::string label;
  std::string symbol;
  uint32_t priority = 0;
  uint32_t min_zoom = 0;
  uint32_t max_zoom = 22;
};

struct LegendGroup {
  std::string layer;
  std::vector<LegendItem> items;
};

std::vector<LegendItem> parse_legend_items(const std::string& text);
std::vector<LegendGroup> group_legend_items(const std::vector<LegendItem>& items, uint32_t zoom);
std::string write_legend(const std::vector<LegendGroup>& groups);
std::vector<LegendItem> merge_legend_items(std::vector<LegendItem> base, const std::vector<LegendItem>& overlay);
uint32_t legend_hash(const std::vector<LegendGroup>& groups);

}  // namespace wayvault

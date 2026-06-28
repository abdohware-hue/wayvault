#include "wayvault/legend_builder.h"

#include <algorithm>
#include <sstream>

#include "wayvault/checksum.h"

namespace wayvault {

namespace {

std::map<std::string, std::string> parse_pairs(const std::string& line) {
  std::map<std::string, std::string> pairs;
  std::istringstream input(line);
  std::string token;
  while (input >> token) {
    size_t eq = token.find('=');
    if (eq != std::string::npos) pairs[token.substr(0, eq)] = token.substr(eq + 1);
  }
  return pairs;
}

std::string item_key(const LegendItem& item) {
  return item.layer + "\n" + item.label + "\n" + item.symbol;
}

}  // namespace

std::vector<LegendItem> parse_legend_items(const std::string& text) {
  std::vector<LegendItem> items;
  std::istringstream input(text);
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line[0] == '#') continue;
    auto pairs = parse_pairs(line);
    LegendItem item;
    item.layer = pairs.count("layer") ? pairs["layer"] : "default";
    item.label = pairs.count("label") ? pairs["label"] : item.layer;
    item.symbol = pairs.count("symbol") ? pairs["symbol"] : "generic";
    item.priority = pairs.count("priority") ? static_cast<uint32_t>(std::stoul(pairs["priority"])) : 0;
    item.min_zoom = pairs.count("min_zoom") ? static_cast<uint32_t>(std::stoul(pairs["min_zoom"])) : 0;
    item.max_zoom = pairs.count("max_zoom") ? static_cast<uint32_t>(std::stoul(pairs["max_zoom"])) : 22;
    if (item.min_zoom > item.max_zoom) std::swap(item.min_zoom, item.max_zoom);
    items.push_back(std::move(item));
  }
  return items;
}

std::vector<LegendGroup> group_legend_items(const std::vector<LegendItem>& items, uint32_t zoom) {
  std::map<std::string, LegendGroup> groups;
  for (const LegendItem& item : items) {
    if (zoom < item.min_zoom || zoom > item.max_zoom) continue;
    LegendGroup& group = groups[item.layer];
    group.layer = item.layer;
    group.items.push_back(item);
  }
  std::vector<LegendGroup> out;
  for (auto& kv : groups) {
    std::sort(kv.second.items.begin(), kv.second.items.end(), [](const LegendItem& a, const LegendItem& b) {
      if (a.priority != b.priority) return a.priority > b.priority;
      return a.label < b.label;
    });
    out.push_back(std::move(kv.second));
  }
  return out;
}

std::string write_legend(const std::vector<LegendGroup>& groups) {
  std::ostringstream out;
  for (const LegendGroup& group : groups) {
    out << '[' << group.layer << "]\n";
    for (const LegendItem& item : group.items) out << item.symbol << " " << item.label << " z" << item.min_zoom << '-' << item.max_zoom << '\n';
  }
  return out.str();
}

std::vector<LegendItem> merge_legend_items(std::vector<LegendItem> base, const std::vector<LegendItem>& overlay) {
  std::map<std::string, size_t> positions;
  for (size_t i = 0; i < base.size(); ++i) positions[item_key(base[i])] = i;
  for (const LegendItem& item : overlay) {
    auto it = positions.find(item_key(item));
    if (it == positions.end()) {
      positions[item_key(item)] = base.size();
      base.push_back(item);
    } else {
      base[it->second] = item;
    }
  }
  return base;
}

uint32_t legend_hash(const std::vector<LegendGroup>& groups) {
  uint32_t state = rotate_mix(0x599e131du, static_cast<uint32_t>(groups.size()));
  for (const LegendGroup& group : groups) {
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(group.layer.data()), group.layer.size()));
    for (const LegendItem& item : group.items) {
      state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(item.label.data()), item.label.size()));
      state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(item.symbol.data()), item.symbol.size()));
      state = rotate_mix(state, item.priority);
      state = rotate_mix(state, item.min_zoom);
      state = rotate_mix(state, item.max_zoom);
    }
  }
  return state;
}

}  // namespace wayvault

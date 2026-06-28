#include "wayvault/spatial_index.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <sstream>

#include "wayvault/checksum.h"

namespace wayvault {

namespace {

double area(Bounds2 b) {
  if (!b.valid || b.max_x <= b.min_x || b.max_y <= b.min_y) return 0.0;
  return (b.max_x - b.min_x) * (b.max_y - b.min_y);
}

Bounds2 intersection(Bounds2 a, Bounds2 b) {
  if (!a.valid || !b.valid) return {};
  Bounds2 out{std::max(a.min_x, b.min_x), std::max(a.min_y, b.min_y),
              std::min(a.max_x, b.max_x), std::min(a.max_y, b.max_y), true};
  if (out.max_x < out.min_x || out.max_y < out.min_y) return {};
  return out;
}

uint32_t layer_bit(uint32_t layer) {
  return layer >= 31 ? 0x80000000u : (1u << layer);
}

std::vector<std::string> split_bar(const std::string& line) {
  std::vector<std::string> parts;
  std::string cur;
  for (char c : line) {
    if (c == '|') {
      parts.push_back(cur);
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  parts.push_back(cur);
  return parts;
}

}  // namespace

SpatialIndex::SpatialIndex(double cell_size) : cell_size_(cell_size > 1.0 ? cell_size : 1.0) {}

std::pair<int, int> SpatialIndex::cell_for(double x, double y) const {
  return {static_cast<int>(std::floor(x / cell_size_)), static_cast<int>(std::floor(y / cell_size_))};
}

std::vector<std::pair<int, int>> SpatialIndex::cells_for(Bounds2 bounds) const {
  std::vector<std::pair<int, int>> cells;
  if (!bounds.valid) return cells;
  auto min_cell = cell_for(bounds.min_x, bounds.min_y);
  auto max_cell = cell_for(bounds.max_x, bounds.max_y);
  for (int y = min_cell.second; y <= max_cell.second; ++y) {
    for (int x = min_cell.first; x <= max_cell.first; ++x) {
      cells.push_back({x, y});
      if (cells.size() > 4096) return cells;
    }
  }
  return cells;
}

bool SpatialIndex::insert(SpatialFeature feature) {
  if (feature.id.empty() || !feature.bounds.valid) return false;
  remove(feature.id);
  features_[feature.id] = feature;
  for (const auto& cell : cells_for(feature.bounds)) grid_.emplace(cell, feature.id);
  return true;
}

bool SpatialIndex::remove(const std::string& id) {
  auto existing = features_.find(id);
  if (existing == features_.end()) return false;
  features_.erase(existing);
  for (auto it = grid_.begin(); it != grid_.end();) {
    if (it->second == id) it = grid_.erase(it);
    else ++it;
  }
  return true;
}

std::vector<SpatialHit> SpatialIndex::search(const SpatialQuery& query) const {
  std::vector<SpatialHit> hits;
  if (!query.bounds.valid) return hits;
  std::set<std::string> candidate_ids;
  for (const auto& cell : cells_for(query.bounds)) {
    auto range = grid_.equal_range(cell);
    for (auto it = range.first; it != range.second; ++it) candidate_ids.insert(it->second);
  }
  for (const std::string& id : candidate_ids) {
    auto fit = features_.find(id);
    if (fit == features_.end()) continue;
    const SpatialFeature& feature = fit->second;
    if ((layer_bit(feature.layer) & query.layer_mask) == 0) continue;
    Bounds2 overlap = intersection(query.bounds, feature.bounds);
    double overlap_area = area(overlap);
    if (overlap_area <= 0.0) continue;
    double feature_area = std::max(1.0, area(feature.bounds));
    hits.push_back({feature, overlap_area / feature_area});
  }
  std::sort(hits.begin(), hits.end(), [](const SpatialHit& a, const SpatialHit& b) {
    if (a.score != b.score) return a.score > b.score;
    if (a.feature.layer != b.feature.layer) return a.feature.layer < b.feature.layer;
    return a.feature.id < b.feature.id;
  });
  if (hits.size() > query.limit) hits.resize(query.limit);
  return hits;
}

std::vector<SpatialFeature> SpatialIndex::all_features() const {
  std::vector<SpatialFeature> out;
  for (const auto& kv : features_) out.push_back(kv.second);
  return out;
}

SpatialIndex build_spatial_index(const std::vector<SpatialFeature>& features, double cell_size) {
  SpatialIndex index(cell_size);
  for (SpatialFeature feature : features) index.insert(std::move(feature));
  return index;
}

std::vector<SpatialFeature> parse_spatial_features(const std::string& text) {
  std::vector<SpatialFeature> features;
  std::istringstream input(text);
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line[0] == '#') continue;
    auto parts = split_bar(line);
    if (parts.size() < 6) continue;
    SpatialFeature feature;
    feature.id = parts[0];
    feature.bounds = {std::stod(parts[1]), std::stod(parts[2]), std::stod(parts[3]), std::stod(parts[4]), true};
    feature.layer = static_cast<uint32_t>(std::stoul(parts[5]));
    if (parts.size() > 6) feature.flags = static_cast<uint32_t>(std::stoul(parts[6]));
    if (feature.bounds.max_x < feature.bounds.min_x) std::swap(feature.bounds.max_x, feature.bounds.min_x);
    if (feature.bounds.max_y < feature.bounds.min_y) std::swap(feature.bounds.max_y, feature.bounds.min_y);
    features.push_back(feature);
  }
  return features;
}

std::string summarize_spatial_index(const SpatialIndex& index) {
  std::vector<SpatialFeature> features = index.all_features();
  Bounds2 total;
  std::map<uint32_t, size_t> by_layer;
  for (const SpatialFeature& feature : features) {
    by_layer[feature.layer]++;
    if (!total.valid) total = feature.bounds;
    else total = expand_bounds({std::min(total.min_x, feature.bounds.min_x), std::min(total.min_y, feature.bounds.min_y),
                                std::max(total.max_x, feature.bounds.max_x), std::max(total.max_y, feature.bounds.max_y), true}, 0.0);
  }
  std::ostringstream out;
  out << "features=" << features.size() << " cell=" << index.cell_size();
  if (total.valid) out << " bounds=" << format_bounds(total);
  for (const auto& kv : by_layer) out << " layer" << kv.first << '=' << kv.second;
  return out.str();
}

uint32_t spatial_index_hash(const SpatialIndex& index) {
  uint32_t state = rotate_mix(0x71a801a3u, static_cast<uint32_t>(index.size()));
  for (const SpatialFeature& feature : index.all_features()) {
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(feature.id.data()), feature.id.size()));
    state = rotate_mix(state, feature.layer);
    state = rotate_mix(state, static_cast<uint32_t>(std::llround(feature.bounds.min_x * 8.0)));
    state = rotate_mix(state, static_cast<uint32_t>(std::llround(feature.bounds.min_y * 8.0)));
    state = rotate_mix(state, static_cast<uint32_t>(std::llround(feature.bounds.max_x * 8.0)));
    state = rotate_mix(state, static_cast<uint32_t>(std::llround(feature.bounds.max_y * 8.0)));
  }
  return state;
}

}  // namespace wayvault

#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "wayvault/geometry.h"

namespace wayvault {

struct SpatialFeature {
  std::string id;
  Bounds2 bounds;
  uint32_t layer = 0;
  uint32_t flags = 0;
};

struct SpatialQuery {
  Bounds2 bounds;
  uint32_t layer_mask = 0xffffffffu;
  size_t limit = 1024;
};

struct SpatialHit {
  SpatialFeature feature;
  double score = 0.0;
};

class SpatialIndex {
 public:
  explicit SpatialIndex(double cell_size = 256.0);
  bool insert(SpatialFeature feature);
  bool remove(const std::string& id);
  std::vector<SpatialHit> search(const SpatialQuery& query) const;
  std::vector<SpatialFeature> all_features() const;
  size_t size() const { return features_.size(); }
  double cell_size() const { return cell_size_; }

 private:
  std::pair<int, int> cell_for(double x, double y) const;
  std::vector<std::pair<int, int>> cells_for(Bounds2 bounds) const;
  double cell_size_ = 256.0;
  std::map<std::string, SpatialFeature> features_;
  std::multimap<std::pair<int, int>, std::string> grid_;
};

SpatialIndex build_spatial_index(const std::vector<SpatialFeature>& features, double cell_size = 256.0);
std::vector<SpatialFeature> parse_spatial_features(const std::string& text);
std::string summarize_spatial_index(const SpatialIndex& index);
uint32_t spatial_index_hash(const SpatialIndex& index);

}  // namespace wayvault

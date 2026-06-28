#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "wayvault/geometry.h"

namespace wayvault {

struct Annotation {
  std::string id;
  std::string layer;
  Point2 anchor;
  std::map<std::string, std::string> properties;
  uint32_t min_zoom = 0;
  uint32_t max_zoom = 22;
};

struct AnnotationLayer {
  std::string name;
  std::vector<Annotation> annotations;
};

struct AnnotationSummary {
  size_t count = 0;
  Bounds2 bounds;
  std::map<std::string, size_t> by_layer;
  std::vector<std::string> duplicate_ids;
};

AnnotationLayer parse_annotation_layer(const std::string& text, const std::string& default_layer = "default");
std::string write_annotation_layer(const AnnotationLayer& layer);
std::vector<Annotation> filter_annotations(const AnnotationLayer& layer, Bounds2 bounds, uint32_t zoom);
AnnotationSummary summarize_annotations(const AnnotationLayer& layer);
void merge_annotation_layer(AnnotationLayer& target, const AnnotationLayer& source, bool replace_existing);
std::vector<Annotation> rank_annotations_by_distance(const AnnotationLayer& layer, Point2 focus, size_t limit);
uint32_t annotation_layer_hash(const AnnotationLayer& layer);

}  // namespace wayvault

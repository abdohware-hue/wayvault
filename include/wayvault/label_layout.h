#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "wayvault/geometry.h"

namespace wayvault {

struct LabelAnchor {
  Point2 point;
  std::string text;
  uint8_t layer = 0;
  int priority = 0;
  double min_scale = 0.0;
};

struct LabelCandidate {
  Point2 min;
  Point2 max;
  Point2 anchor;
  std::string text;
  int priority = 0;
  double score = 0.0;
};

struct LabelPlacement {
  LabelCandidate candidate;
  bool placed = false;
};

Bounds2 candidate_bounds(const LabelCandidate& candidate);
std::vector<LabelCandidate> make_label_candidates(const LabelAnchor& anchor, double text_height);
std::vector<LabelPlacement> place_labels(const std::vector<LabelAnchor>& anchors, Bounds2 viewport, double text_height);
bool candidate_overlaps(const LabelCandidate& a, const LabelCandidate& b, double padding);
uint32_t placement_hash(const std::vector<LabelPlacement>& placements);
std::string summarize_placements(const std::vector<LabelPlacement>& placements);

}  // namespace wayvault

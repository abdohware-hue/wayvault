#include "wayvault/label_layout.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

#include "wayvault/checksum.h"

namespace wayvault {

namespace {

Point2 offset(Point2 p, double dx, double dy) {
  return {p.x + dx, p.y + dy};
}

LabelCandidate build_candidate(const LabelAnchor& anchor, double width, double height, double ox, double oy, double bias) {
  LabelCandidate c;
  c.anchor = anchor.point;
  c.text = anchor.text;
  c.priority = anchor.priority;
  c.min = offset(anchor.point, ox, oy);
  c.max = offset(c.min, width, height);
  double anchor_penalty = std::abs(ox) + std::abs(oy);
  c.score = static_cast<double>(anchor.priority) * 1000.0 - anchor_penalty + bias;
  return c;
}

bool inside(Bounds2 viewport, const LabelCandidate& c) {
  return viewport.valid && c.min.x >= viewport.min_x && c.max.x <= viewport.max_x &&
         c.min.y >= viewport.min_y && c.max.y <= viewport.max_y;
}

}  // namespace

Bounds2 candidate_bounds(const LabelCandidate& candidate) {
  Bounds2 b;
  b.min_x = candidate.min.x;
  b.min_y = candidate.min.y;
  b.max_x = candidate.max.x;
  b.max_y = candidate.max.y;
  b.valid = true;
  return b;
}

std::vector<LabelCandidate> make_label_candidates(const LabelAnchor& anchor, double text_height) {
  double height = std::max(1.0, text_height);
  double width = std::max(height, height * (0.48 * static_cast<double>(anchor.text.size()) + 0.8));
  double gap = height * 0.35;
  std::vector<LabelCandidate> out;
  out.push_back(build_candidate(anchor, width, height, gap, -height * 0.5, 50.0));
  out.push_back(build_candidate(anchor, width, height, -width - gap, -height * 0.5, 40.0));
  out.push_back(build_candidate(anchor, width, height, -width * 0.5, gap, 30.0));
  out.push_back(build_candidate(anchor, width, height, -width * 0.5, -height - gap, 20.0));
  out.push_back(build_candidate(anchor, width, height, gap, gap, 10.0));
  out.push_back(build_candidate(anchor, width, height, -width - gap, gap, 5.0));
  return out;
}

bool candidate_overlaps(const LabelCandidate& a, const LabelCandidate& b, double padding) {
  Bounds2 ba = expand_bounds(candidate_bounds(a), padding);
  Bounds2 bb = expand_bounds(candidate_bounds(b), padding);
  return bounds_intersect(ba, bb);
}

std::vector<LabelPlacement> place_labels(const std::vector<LabelAnchor>& anchors, Bounds2 viewport, double text_height) {
  std::vector<LabelAnchor> sorted = anchors;
  std::stable_sort(sorted.begin(), sorted.end(), [](const LabelAnchor& a, const LabelAnchor& b) {
    if (a.priority != b.priority) return a.priority > b.priority;
    return a.text < b.text;
  });
  std::vector<LabelCandidate> accepted;
  std::vector<LabelPlacement> placements;
  for (const LabelAnchor& anchor : sorted) {
    LabelPlacement placement;
    std::vector<LabelCandidate> candidates = make_label_candidates(anchor, text_height);
    std::stable_sort(candidates.begin(), candidates.end(), [](const LabelCandidate& a, const LabelCandidate& b) {
      return a.score > b.score;
    });
    for (const LabelCandidate& c : candidates) {
      if (!inside(viewport, c)) continue;
      bool overlaps = false;
      for (const LabelCandidate& used : accepted) {
        if (candidate_overlaps(c, used, text_height * 0.25)) {
          overlaps = true;
          break;
        }
      }
      if (!overlaps) {
        placement.candidate = c;
        placement.placed = true;
        accepted.push_back(c);
        break;
      }
    }
    if (!placement.placed && !candidates.empty()) placement.candidate = candidates.front();
    placements.push_back(placement);
  }
  return placements;
}

uint32_t placement_hash(const std::vector<LabelPlacement>& placements) {
  uint32_t state = 2166136261u;
  for (const LabelPlacement& placement : placements) {
    state = rotate_mix(state, placement.placed ? 0x9e3779b9u : 0x7f4a7c15u);
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(placement.candidate.text.data()), placement.candidate.text.size()));
    state = rotate_mix(state, static_cast<uint32_t>(placement.candidate.min.x * 16.0));
    state = rotate_mix(state, static_cast<uint32_t>(placement.candidate.min.y * 16.0));
  }
  return state;
}

std::string summarize_placements(const std::vector<LabelPlacement>& placements) {
  size_t placed = 0;
  for (const LabelPlacement& p : placements) if (p.placed) ++placed;
  std::ostringstream out;
  out << "labels " << placed << "/" << placements.size() << " placed";
  for (const LabelPlacement& p : placements) {
    out << "\n  " << (p.placed ? "ok " : "drop ") << p.candidate.text
        << " @ " << std::fixed << std::setprecision(1)
        << p.candidate.min.x << "," << p.candidate.min.y;
  }
  return out.str();
}

}  // namespace wayvault

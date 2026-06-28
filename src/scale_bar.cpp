#include "wayvault/scale_bar.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace wayvault {

std::string format_distance_label(double meters) {
  std::ostringstream out;
  if (meters >= 1000.0) {
    double km = meters / 1000.0;
    if (km >= 10.0 || std::abs(km - std::round(km)) < 0.01) {
      out << static_cast<int>(std::round(km)) << " km";
    } else {
      out.setf(std::ios::fixed);
      out.precision(1);
      out << km << " km";
    }
  } else {
    out << static_cast<int>(std::round(meters)) << " m";
  }
  return out.str();
}

std::vector<double> nice_distance_candidates(double max_meters) {
  std::vector<double> candidates;
  if (max_meters <= 0.0) {
    return candidates;
  }
  double base = 1.0;
  while (base <= max_meters * 10.0) {
    for (double step : {1.0, 2.0, 5.0}) {
      double value = step * base;
      if (value <= max_meters) {
        candidates.push_back(value);
      }
    }
    base *= 10.0;
  }
  std::sort(candidates.begin(), candidates.end());
  candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());
  return candidates;
}

ScaleBarPlan make_scale_bar_plan(double meters_per_pixel, uint32_t target_pixels) {
  ScaleBarPlan plan;
  plan.meters_per_pixel = meters_per_pixel <= 0.0 ? 1.0 : meters_per_pixel;
  plan.target_pixels = target_pixels == 0 ? 120 : target_pixels;
  double max_meters = plan.meters_per_pixel * static_cast<double>(plan.target_pixels);
  for (double meters : nice_distance_candidates(max_meters)) {
    plan.breaks.push_back(ScaleBreak{meters, format_distance_label(meters)});
  }
  if (plan.breaks.empty()) {
    plan.breaks.push_back(ScaleBreak{plan.meters_per_pixel, format_distance_label(plan.meters_per_pixel)});
  }
  return plan;
}

ScaleBreak choose_scale_break(const ScaleBarPlan& plan) {
  if (plan.breaks.empty()) {
    return ScaleBreak{plan.meters_per_pixel, format_distance_label(plan.meters_per_pixel)};
  }
  double target = plan.meters_per_pixel * static_cast<double>(plan.target_pixels);
  ScaleBreak best = plan.breaks.front();
  for (const ScaleBreak& candidate : plan.breaks) {
    if (candidate.meters <= target) {
      best = candidate;
    }
  }
  return best;
}

uint32_t scale_bar_pixel_width(const ScaleBarPlan& plan, const ScaleBreak& chosen) {
  if (plan.meters_per_pixel <= 0.0) {
    return 0;
  }
  double pixels = chosen.meters / plan.meters_per_pixel;
  if (pixels < 1.0) {
    return 1;
  }
  if (pixels > static_cast<double>(UINT32_MAX)) {
    return UINT32_MAX;
  }
  return static_cast<uint32_t>(std::round(pixels));
}

}  // namespace wayvault

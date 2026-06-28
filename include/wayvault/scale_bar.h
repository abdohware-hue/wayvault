#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace wayvault {

struct ScaleBreak {
  double meters = 0.0;
  std::string label;
};

struct ScaleBarPlan {
  double meters_per_pixel = 1.0;
  uint32_t target_pixels = 120;
  std::vector<ScaleBreak> breaks;
};

ScaleBarPlan make_scale_bar_plan(double meters_per_pixel, uint32_t target_pixels);
ScaleBreak choose_scale_break(const ScaleBarPlan& plan);
std::string format_distance_label(double meters);
std::vector<double> nice_distance_candidates(double max_meters);
uint32_t scale_bar_pixel_width(const ScaleBarPlan& plan, const ScaleBreak& chosen);

}  // namespace wayvault

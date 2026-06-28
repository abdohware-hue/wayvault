#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "wayvault/style_sheet.h"

namespace wayvault {

struct RampStop {
  double value = 0.0;
  ColorRGBA color;
};

struct ColorRamp {
  std::string name;
  std::vector<RampStop> stops;
  ColorRGBA fallback;
};

ColorRamp parse_color_ramp(const std::string& text);
ColorRGBA sample_color_ramp(const ColorRamp& ramp, double value);
std::vector<ColorRGBA> build_color_lut(const ColorRamp& ramp, size_t size, double min_value, double max_value);
std::string render_color_ramp(const ColorRamp& ramp);
uint32_t color_ramp_hash(const ColorRamp& ramp);

}  // namespace wayvault

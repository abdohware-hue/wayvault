#include "wayvault/color_ramp.h"

#include <algorithm>
#include <cmath>
#include <sstream>

#include "wayvault/checksum.h"
#include "wayvault/error.h"

namespace wayvault {

namespace {

double clamp_unit(double value) {
  if (value < 0.0) {
    return 0.0;
  }
  if (value > 1.0) {
    return 1.0;
  }
  return value;
}

uint8_t mix_channel(uint8_t a, uint8_t b, double t) {
  double v = static_cast<double>(a) + (static_cast<double>(b) - static_cast<double>(a)) * t;
  return static_cast<uint8_t>(std::lround(std::max(0.0, std::min(255.0, v))));
}

ColorRGBA mix_color(ColorRGBA a, ColorRGBA b, double t) {
  t = clamp_unit(t);
  ColorRGBA out;
  out.r = mix_channel(a.r, b.r, t);
  out.g = mix_channel(a.g, b.g, t);
  out.b = mix_channel(a.b, b.b, t);
  out.a = mix_channel(a.a, b.a, t);
  return out;
}

std::string trim_ramp(const std::string& value) {
  size_t first = 0;
  while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first]))) {
    ++first;
  }
  size_t last = value.size();
  while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1]))) {
    --last;
  }
  return value.substr(first, last - first);
}

}  // namespace

ColorRamp parse_color_ramp(const std::string& text) {
  ColorRamp ramp;
  ramp.fallback = {0, 0, 0, 255};
  std::istringstream input(text);
  std::string line;
  while (std::getline(input, line)) {
    line = trim_ramp(line);
    if (line.empty() || line[0] == '#') {
      continue;
    }
    size_t eq = line.find('=');
    if (eq != std::string::npos && line.substr(0, eq) == "name") {
      ramp.name = trim_ramp(line.substr(eq + 1));
      continue;
    }
    if (eq != std::string::npos && line.substr(0, eq) == "fallback") {
      ramp.fallback = parse_color(trim_ramp(line.substr(eq + 1)));
      continue;
    }
    size_t colon = line.find(':');
    if (colon == std::string::npos) {
      throw ParseError("color ramp stop needs value:color");
    }
    RampStop stop;
    stop.value = std::stod(trim_ramp(line.substr(0, colon)));
    stop.color = parse_color(trim_ramp(line.substr(colon + 1)));
    ramp.stops.push_back(stop);
  }
  std::stable_sort(ramp.stops.begin(), ramp.stops.end(), [](const RampStop& a, const RampStop& b) {
    return a.value < b.value;
  });
  return ramp;
}

ColorRGBA sample_color_ramp(const ColorRamp& ramp, double value) {
  if (ramp.stops.empty()) {
    return ramp.fallback;
  }
  if (value <= ramp.stops.front().value) {
    return ramp.stops.front().color;
  }
  if (value >= ramp.stops.back().value) {
    return ramp.stops.back().color;
  }
  for (size_t i = 1; i < ramp.stops.size(); ++i) {
    const RampStop& left = ramp.stops[i - 1];
    const RampStop& right = ramp.stops[i];
    if (value <= right.value) {
      double span = right.value - left.value;
      double t = span == 0.0 ? 0.0 : (value - left.value) / span;
      return mix_color(left.color, right.color, t);
    }
  }
  return ramp.fallback;
}

std::vector<ColorRGBA> build_color_lut(const ColorRamp& ramp, size_t size, double min_value, double max_value) {
  std::vector<ColorRGBA> lut;
  if (size == 0) {
    return lut;
  }
  lut.reserve(size);
  for (size_t i = 0; i < size; ++i) {
    double t = size == 1 ? 0.0 : static_cast<double>(i) / static_cast<double>(size - 1);
    double value = min_value + (max_value - min_value) * t;
    lut.push_back(sample_color_ramp(ramp, value));
  }
  return lut;
}

std::string render_color_ramp(const ColorRamp& ramp) {
  std::ostringstream out;
  out << "name=" << ramp.name << "\n";
  out << "fallback=" << color_to_hex(ramp.fallback, ramp.fallback.a != 255) << "\n";
  for (const RampStop& stop : ramp.stops) {
    out << stop.value << ":" << color_to_hex(stop.color, stop.color.a != 255) << "\n";
  }
  return out.str();
}

uint32_t color_ramp_hash(const ColorRamp& ramp) {
  uint32_t state = fnv1a32(reinterpret_cast<const uint8_t*>(ramp.name.data()), ramp.name.size());
  for (const RampStop& stop : ramp.stops) {
    uint32_t packed = static_cast<uint32_t>(stop.color.r) |
                      (static_cast<uint32_t>(stop.color.g) << 8) |
                      (static_cast<uint32_t>(stop.color.b) << 16) |
                      (static_cast<uint32_t>(stop.color.a) << 24);
    state = rotate_mix(state, static_cast<uint32_t>(stop.value * 1000000.0));
    state = rotate_mix(state, packed);
  }
  return state;
}

}  // namespace wayvault

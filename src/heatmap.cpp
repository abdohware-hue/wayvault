#include "wayvault/heatmap.h"

#include <algorithm>
#include <cmath>

#include "wayvault/checksum.h"

namespace wayvault {

std::vector<WeightedPoint> normalize_weights(std::vector<WeightedPoint> points) {
  double max_weight = 0.0;
  for (const WeightedPoint& point : points) max_weight = std::max(max_weight, std::abs(point.weight));
  if (max_weight <= 1e-9) return points;
  for (WeightedPoint& point : points) point.weight /= max_weight;
  return points;
}

double heat_at_point(const std::vector<WeightedPoint>& points, Point2 point, double radius) {
  if (radius <= 0.0) return 0.0;
  double heat = 0.0;
  double r2 = radius * radius;
  for (const WeightedPoint& weighted : points) {
    double dx = weighted.point.x - point.x;
    double dy = weighted.point.y - point.y;
    double d2 = dx * dx + dy * dy;
    if (d2 > r2) continue;
    double k = 1.0 - d2 / r2;
    heat += weighted.weight * k * k;
  }
  return heat;
}

GrayImage render_heatmap(const std::vector<WeightedPoint>& points, Bounds2 bounds, uint32_t width, uint32_t height, double radius) {
  GrayImage image = make_image(width, height, 0);
  if (!bounds.valid || width == 0 || height == 0) return image;
  std::vector<WeightedPoint> normalized = normalize_weights(points);
  for (uint32_t y = 0; y < height; ++y) {
    double py = bounds.min_y + (bounds.max_y - bounds.min_y) * (static_cast<double>(y) + 0.5) / height;
    for (uint32_t x = 0; x < width; ++x) {
      double px = bounds.min_x + (bounds.max_x - bounds.min_x) * (static_cast<double>(x) + 0.5) / width;
      double heat = std::max(0.0, std::min(1.0, heat_at_point(normalized, Point2{px, py}, radius)));
      image.pixels[static_cast<size_t>(y) * width + x] = static_cast<uint8_t>(std::lround(heat * 255.0));
    }
  }
  return image;
}

uint32_t heatmap_signature(const GrayImage& image) {
  return rotate_mix(image_hash(image), static_cast<uint32_t>(analyze_image(image).mean * 1000.0));
}

}  // namespace wayvault

#pragma once

#include <cstdint>
#include <vector>

#include "wayvault/geometry.h"
#include "wayvault/raster_ops.h"

namespace wayvault {

struct WeightedPoint {
  Point2 point;
  double weight = 1.0;
};

GrayImage render_heatmap(const std::vector<WeightedPoint>& points, Bounds2 bounds, uint32_t width, uint32_t height, double radius);
std::vector<WeightedPoint> normalize_weights(std::vector<WeightedPoint> points);
double heat_at_point(const std::vector<WeightedPoint>& points, Point2 point, double radius);
uint32_t heatmap_signature(const GrayImage& image);

}  // namespace wayvault

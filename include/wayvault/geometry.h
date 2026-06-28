#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace wayvault {

struct Point2 {
  double x = 0.0;
  double y = 0.0;
};

struct Bounds2 {
  double min_x = 0.0;
  double min_y = 0.0;
  double max_x = 0.0;
  double max_y = 0.0;
  bool valid = false;
};

struct PolylineStats {
  size_t point_count = 0;
  double length = 0.0;
  Bounds2 bounds;
  double chord = 0.0;
  double sinuosity = 0.0;
  uint32_t shape_hash = 0;
};

struct PolygonSummary {
  double signed_area = 0.0;
  double perimeter = 0.0;
  Point2 centroid;
  Bounds2 bounds;
  bool clockwise = false;
  bool simple_enough = true;
};

double distance(Point2 a, Point2 b);
double dot(Point2 a, Point2 b);
double cross(Point2 a, Point2 b, Point2 c);
Bounds2 bounds_for_points(const std::vector<Point2>& points);
bool bounds_intersect(const Bounds2& a, const Bounds2& b);
bool bounds_contains(const Bounds2& bounds, Point2 p);
Bounds2 expand_bounds(Bounds2 bounds, double amount);
double path_length(const std::vector<Point2>& points);
PolylineStats analyze_polyline(const std::vector<Point2>& points);
double polygon_signed_area(const std::vector<Point2>& ring);
double polygon_perimeter(const std::vector<Point2>& ring);
Point2 polygon_centroid(const std::vector<Point2>& ring);
PolygonSummary summarize_polygon(const std::vector<Point2>& ring);
bool point_in_polygon(const std::vector<Point2>& ring, Point2 point);
bool segments_intersect(Point2 a, Point2 b, Point2 c, Point2 d);
std::vector<Point2> simplify_polyline(const std::vector<Point2>& points, double tolerance);
std::vector<Point2> resample_polyline(const std::vector<Point2>& points, double spacing);
std::vector<Point2> clip_polyline_to_bounds(const std::vector<Point2>& points, Bounds2 bounds);
std::vector<Point2> densify_polyline(const std::vector<Point2>& points, double max_step);
uint32_t hash_points(const std::vector<Point2>& points);
std::string format_bounds(Bounds2 bounds);

}  // namespace wayvault

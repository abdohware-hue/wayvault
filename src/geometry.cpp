#include "wayvault/geometry.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>

#include "wayvault/checksum.h"

namespace wayvault {

namespace {

constexpr double kEpsilon = 1e-9;

Point2 subtract(Point2 a, Point2 b) {
  return {a.x - b.x, a.y - b.y};
}

double clamp01(double v) {
  if (v < 0.0) return 0.0;
  if (v > 1.0) return 1.0;
  return v;
}

double distance_to_segment(Point2 p, Point2 a, Point2 b) {
  Point2 ab = subtract(b, a);
  double len2 = ab.x * ab.x + ab.y * ab.y;
  if (len2 <= kEpsilon) {
    return distance(p, a);
  }
  double t = ((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / len2;
  t = clamp01(t);
  Point2 q{a.x + t * ab.x, a.y + t * ab.y};
  return distance(p, q);
}

void simplify_range(const std::vector<Point2>& points, size_t first, size_t last, double tolerance,
                    std::vector<char>& keep) {
  if (last <= first + 1) {
    return;
  }
  double best = -1.0;
  size_t best_index = first;
  for (size_t i = first + 1; i < last; ++i) {
    double d = distance_to_segment(points[i], points[first], points[last]);
    if (d > best) {
      best = d;
      best_index = i;
    }
  }
  if (best > tolerance) {
    keep[best_index] = 1;
    simplify_range(points, first, best_index, tolerance, keep);
    simplify_range(points, best_index, last, tolerance, keep);
  }
}

Point2 interpolate(Point2 a, Point2 b, double t) {
  return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}

bool inside_left(Point2 p, Bounds2 b) { return p.x >= b.min_x; }
bool inside_right(Point2 p, Bounds2 b) { return p.x <= b.max_x; }
bool inside_bottom(Point2 p, Bounds2 b) { return p.y >= b.min_y; }
bool inside_top(Point2 p, Bounds2 b) { return p.y <= b.max_y; }

Point2 intersect_vertical(Point2 a, Point2 b, double x) {
  double dx = b.x - a.x;
  if (std::abs(dx) <= kEpsilon) return {x, a.y};
  double t = (x - a.x) / dx;
  return interpolate(a, b, t);
}

Point2 intersect_horizontal(Point2 a, Point2 b, double y) {
  double dy = b.y - a.y;
  if (std::abs(dy) <= kEpsilon) return {a.x, y};
  double t = (y - a.y) / dy;
  return interpolate(a, b, t);
}

void append_if_distinct(std::vector<Point2>& out, Point2 p) {
  if (out.empty() || distance(out.back(), p) > kEpsilon) {
    out.push_back(p);
  }
}

template <typename Inside, typename Intersect>
std::vector<Point2> clip_edge(const std::vector<Point2>& input, Inside inside, Intersect intersect) {
  std::vector<Point2> output;
  if (input.empty()) return output;
  Point2 prev = input.back();
  bool prev_inside = inside(prev);
  for (Point2 cur : input) {
    bool cur_inside = inside(cur);
    if (cur_inside) {
      if (!prev_inside) append_if_distinct(output, intersect(prev, cur));
      append_if_distinct(output, cur);
    } else if (prev_inside) {
      append_if_distinct(output, intersect(prev, cur));
    }
    prev = cur;
    prev_inside = cur_inside;
  }
  return output;
}

}  // namespace

double distance(Point2 a, Point2 b) {
  double dx = a.x - b.x;
  double dy = a.y - b.y;
  return std::sqrt(dx * dx + dy * dy);
}

double dot(Point2 a, Point2 b) {
  return a.x * b.x + a.y * b.y;
}

double cross(Point2 a, Point2 b, Point2 c) {
  return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

Bounds2 bounds_for_points(const std::vector<Point2>& points) {
  Bounds2 b;
  if (points.empty()) return b;
  b.min_x = b.max_x = points.front().x;
  b.min_y = b.max_y = points.front().y;
  b.valid = true;
  for (const Point2& p : points) {
    b.min_x = std::min(b.min_x, p.x);
    b.max_x = std::max(b.max_x, p.x);
    b.min_y = std::min(b.min_y, p.y);
    b.max_y = std::max(b.max_y, p.y);
  }
  return b;
}

bool bounds_intersect(const Bounds2& a, const Bounds2& b) {
  if (!a.valid || !b.valid) return false;
  if (a.max_x < b.min_x || b.max_x < a.min_x) return false;
  if (a.max_y < b.min_y || b.max_y < a.min_y) return false;
  return true;
}

bool bounds_contains(const Bounds2& bounds, Point2 p) {
  if (!bounds.valid) return false;
  return p.x >= bounds.min_x && p.x <= bounds.max_x && p.y >= bounds.min_y && p.y <= bounds.max_y;
}

Bounds2 expand_bounds(Bounds2 bounds, double amount) {
  if (!bounds.valid) return bounds;
  bounds.min_x -= amount;
  bounds.min_y -= amount;
  bounds.max_x += amount;
  bounds.max_y += amount;
  return bounds;
}

double path_length(const std::vector<Point2>& points) {
  double total = 0.0;
  for (size_t i = 1; i < points.size(); ++i) {
    total += distance(points[i - 1], points[i]);
  }
  return total;
}

PolylineStats analyze_polyline(const std::vector<Point2>& points) {
  PolylineStats stats;
  stats.point_count = points.size();
  stats.length = path_length(points);
  stats.bounds = bounds_for_points(points);
  if (points.size() >= 2) {
    stats.chord = distance(points.front(), points.back());
    stats.sinuosity = stats.chord > kEpsilon ? stats.length / stats.chord : 0.0;
  }
  stats.shape_hash = hash_points(points);
  return stats;
}

double polygon_signed_area(const std::vector<Point2>& ring) {
  if (ring.size() < 3) return 0.0;
  double area2 = 0.0;
  for (size_t i = 0; i < ring.size(); ++i) {
    const Point2& a = ring[i];
    const Point2& b = ring[(i + 1) % ring.size()];
    area2 += a.x * b.y - b.x * a.y;
  }
  return area2 * 0.5;
}

double polygon_perimeter(const std::vector<Point2>& ring) {
  if (ring.size() < 2) return 0.0;
  double total = 0.0;
  for (size_t i = 0; i < ring.size(); ++i) {
    total += distance(ring[i], ring[(i + 1) % ring.size()]);
  }
  return total;
}

Point2 polygon_centroid(const std::vector<Point2>& ring) {
  Point2 c;
  double twice_area = 0.0;
  if (ring.size() < 3) return c;
  for (size_t i = 0; i < ring.size(); ++i) {
    const Point2& a = ring[i];
    const Point2& b = ring[(i + 1) % ring.size()];
    double cross_term = a.x * b.y - b.x * a.y;
    twice_area += cross_term;
    c.x += (a.x + b.x) * cross_term;
    c.y += (a.y + b.y) * cross_term;
  }
  if (std::abs(twice_area) <= kEpsilon) {
    Bounds2 b = bounds_for_points(ring);
    return {(b.min_x + b.max_x) * 0.5, (b.min_y + b.max_y) * 0.5};
  }
  c.x /= 3.0 * twice_area;
  c.y /= 3.0 * twice_area;
  return c;
}

PolygonSummary summarize_polygon(const std::vector<Point2>& ring) {
  PolygonSummary summary;
  summary.signed_area = polygon_signed_area(ring);
  summary.perimeter = polygon_perimeter(ring);
  summary.centroid = polygon_centroid(ring);
  summary.bounds = bounds_for_points(ring);
  summary.clockwise = summary.signed_area < 0.0;
  for (size_t i = 0; i + 1 < ring.size(); ++i) {
    for (size_t j = i + 2; j + 1 < ring.size(); ++j) {
      if (i == 0 && j + 1 == ring.size()) continue;
      if (segments_intersect(ring[i], ring[i + 1], ring[j], ring[j + 1])) {
        summary.simple_enough = false;
        return summary;
      }
    }
  }
  return summary;
}

bool point_in_polygon(const std::vector<Point2>& ring, Point2 point) {
  bool inside = false;
  for (size_t i = 0, j = ring.size(); i < ring.size(); j = i++) {
    const Point2& a = ring[i];
    const Point2& b = ring[j == ring.size() ? ring.size() - 1 : j];
    bool crosses = ((a.y > point.y) != (b.y > point.y));
    if (crosses) {
      double x = (b.x - a.x) * (point.y - a.y) / (b.y - a.y + kEpsilon) + a.x;
      if (point.x < x) inside = !inside;
    }
  }
  return inside;
}

bool segments_intersect(Point2 a, Point2 b, Point2 c, Point2 d) {
  double c1 = cross(a, b, c);
  double c2 = cross(a, b, d);
  double c3 = cross(c, d, a);
  double c4 = cross(c, d, b);
  if ((c1 > kEpsilon && c2 < -kEpsilon || c1 < -kEpsilon && c2 > kEpsilon) &&
      (c3 > kEpsilon && c4 < -kEpsilon || c3 < -kEpsilon && c4 > kEpsilon)) {
    return true;
  }
  Bounds2 ab = bounds_for_points({a, b});
  Bounds2 cd = bounds_for_points({c, d});
  if (!bounds_intersect(ab, cd)) return false;
  return std::abs(c1) <= kEpsilon || std::abs(c2) <= kEpsilon ||
         std::abs(c3) <= kEpsilon || std::abs(c4) <= kEpsilon;
}

std::vector<Point2> simplify_polyline(const std::vector<Point2>& points, double tolerance) {
  if (points.size() <= 2 || tolerance <= 0.0) return points;
  std::vector<char> keep(points.size(), 0);
  keep.front() = 1;
  keep.back() = 1;
  simplify_range(points, 0, points.size() - 1, tolerance, keep);
  std::vector<Point2> out;
  for (size_t i = 0; i < points.size(); ++i) {
    if (keep[i]) out.push_back(points[i]);
  }
  return out;
}

std::vector<Point2> resample_polyline(const std::vector<Point2>& points, double spacing) {
  if (points.empty() || spacing <= 0.0) return points;
  std::vector<Point2> out;
  out.push_back(points.front());
  double carry = spacing;
  for (size_t i = 1; i < points.size(); ++i) {
    Point2 a = points[i - 1];
    Point2 b = points[i];
    double segment = distance(a, b);
    while (segment + kEpsilon >= carry && segment > kEpsilon) {
      double t = carry / segment;
      Point2 p = interpolate(a, b, t);
      out.push_back(p);
      a = p;
      segment = distance(a, b);
      carry = spacing;
    }
    carry -= segment;
    if (carry <= kEpsilon) carry = spacing;
  }
  if (distance(out.back(), points.back()) > kEpsilon) out.push_back(points.back());
  return out;
}

std::vector<Point2> clip_polyline_to_bounds(const std::vector<Point2>& points, Bounds2 bounds) {
  if (!bounds.valid || points.empty()) return {};
  std::vector<Point2> out = points;
  out = clip_edge(out, [&](Point2 p) { return inside_left(p, bounds); },
                  [&](Point2 a, Point2 b) { return intersect_vertical(a, b, bounds.min_x); });
  out = clip_edge(out, [&](Point2 p) { return inside_right(p, bounds); },
                  [&](Point2 a, Point2 b) { return intersect_vertical(a, b, bounds.max_x); });
  out = clip_edge(out, [&](Point2 p) { return inside_bottom(p, bounds); },
                  [&](Point2 a, Point2 b) { return intersect_horizontal(a, b, bounds.min_y); });
  out = clip_edge(out, [&](Point2 p) { return inside_top(p, bounds); },
                  [&](Point2 a, Point2 b) { return intersect_horizontal(a, b, bounds.max_y); });
  return out;
}

std::vector<Point2> densify_polyline(const std::vector<Point2>& points, double max_step) {
  if (points.size() < 2 || max_step <= 0.0) return points;
  std::vector<Point2> out;
  out.push_back(points.front());
  for (size_t i = 1; i < points.size(); ++i) {
    double len = distance(points[i - 1], points[i]);
    size_t pieces = static_cast<size_t>(std::ceil(len / max_step));
    for (size_t j = 1; j < pieces; ++j) {
      out.push_back(interpolate(points[i - 1], points[i], static_cast<double>(j) / pieces));
    }
    out.push_back(points[i]);
  }
  return out;
}

uint32_t hash_points(const std::vector<Point2>& points) {
  uint32_t state = 2166136261u;
  for (const Point2& p : points) {
    int64_t qx = static_cast<int64_t>(std::llround(p.x * 1000000.0));
    int64_t qy = static_cast<int64_t>(std::llround(p.y * 1000000.0));
    state = rotate_mix(state, static_cast<uint32_t>(qx));
    state = rotate_mix(state, static_cast<uint32_t>(qx >> 32));
    state = rotate_mix(state, static_cast<uint32_t>(qy));
    state = rotate_mix(state, static_cast<uint32_t>(qy >> 32));
  }
  return state;
}

std::string format_bounds(Bounds2 bounds) {
  if (!bounds.valid) return "empty";
  std::ostringstream out;
  out << std::fixed << std::setprecision(3)
      << bounds.min_x << "," << bounds.min_y << " -> " << bounds.max_x << "," << bounds.max_y;
  return out.str();
}

}  // namespace wayvault

#include "wayvault/viewport.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

#include "wayvault/checksum.h"

namespace wayvault {

namespace {

double clampd(double v, double lo, double hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

uint32_t zoom_floor(double zoom) {
  if (zoom <= 0.0) return 0;
  if (zoom >= 22.0) return 22;
  return static_cast<uint32_t>(std::floor(zoom));
}

ScreenPoint rotate_screen(ScreenPoint p, double degrees) {
  if (std::abs(degrees) < 1e-9) return p;
  double r = degrees * 3.14159265358979323846 / 180.0;
  double c = std::cos(r);
  double s = std::sin(r);
  return {p.x * c - p.y * s, p.x * s + p.y * c};
}

Bounds2 screen_bounds_for_tile(const ViewportState& viewport, TileCoord coord) {
  GeoPoint nw = world_to_lon_lat({coord.x * 256.0, coord.y * 256.0}, coord.z);
  GeoPoint se = world_to_lon_lat({(coord.x + 1) * 256.0, (coord.y + 1) * 256.0}, coord.z);
  std::vector<ScreenPoint> points = {
      project_to_screen(viewport, nw),
      project_to_screen(viewport, {se.lon, nw.lat}),
      project_to_screen(viewport, se),
      project_to_screen(viewport, {nw.lon, se.lat}),
  };
  Bounds2 bounds;
  for (ScreenPoint p : points) {
    if (!bounds.valid) bounds = {p.x, p.y, p.x, p.y, true};
    else {
      bounds.min_x = std::min(bounds.min_x, p.x);
      bounds.min_y = std::min(bounds.min_y, p.y);
      bounds.max_x = std::max(bounds.max_x, p.x);
      bounds.max_y = std::max(bounds.max_y, p.y);
    }
  }
  return bounds;
}

bool intersects_screen(Bounds2 bounds, uint32_t width, uint32_t height) {
  if (!bounds.valid) return false;
  Bounds2 screen{0.0, 0.0, static_cast<double>(width), static_cast<double>(height), true};
  return bounds_intersect(bounds, screen);
}

uint32_t mix_int(double v) {
  long long scaled = static_cast<long long>(std::llround(v * 256.0));
  return static_cast<uint32_t>(scaled) ^ static_cast<uint32_t>(scaled >> 32);
}

}  // namespace

ViewportState normalize_viewport(ViewportState viewport) {
  viewport.center = clamp_geo(viewport.center);
  viewport.zoom = std::min<uint32_t>(viewport.zoom, 22);
  if (viewport.width == 0) viewport.width = 1;
  if (viewport.height == 0) viewport.height = 1;
  while (viewport.bearing_degrees < -180.0) viewport.bearing_degrees += 360.0;
  while (viewport.bearing_degrees > 180.0) viewport.bearing_degrees -= 360.0;
  viewport.pitch_degrees = clampd(viewport.pitch_degrees, 0.0, 65.0);
  return viewport;
}

ScreenPoint project_to_screen(const ViewportState& raw_viewport, GeoPoint point) {
  ViewportState viewport = normalize_viewport(raw_viewport);
  PixelPoint center = lon_lat_to_world(viewport.center, viewport.zoom);
  PixelPoint world = lon_lat_to_world(point, viewport.zoom);
  ScreenPoint delta{world.x - center.x, world.y - center.y};
  delta = rotate_screen(delta, -viewport.bearing_degrees);
  double pitch_scale = std::cos(viewport.pitch_degrees * 3.14159265358979323846 / 180.0);
  if (pitch_scale < 0.25) pitch_scale = 0.25;
  delta.y *= pitch_scale;
  return {delta.x + viewport.width / 2.0, delta.y + viewport.height / 2.0};
}

GeoPoint unproject_from_screen(const ViewportState& raw_viewport, ScreenPoint point) {
  ViewportState viewport = normalize_viewport(raw_viewport);
  ScreenPoint delta{point.x - viewport.width / 2.0, point.y - viewport.height / 2.0};
  double pitch_scale = std::cos(viewport.pitch_degrees * 3.14159265358979323846 / 180.0);
  if (pitch_scale < 0.25) pitch_scale = 0.25;
  delta.y /= pitch_scale;
  delta = rotate_screen(delta, viewport.bearing_degrees);
  PixelPoint center = lon_lat_to_world(viewport.center, viewport.zoom);
  return world_to_lon_lat({center.x + delta.x, center.y + delta.y}, viewport.zoom);
}

ViewportState pan_viewport(ViewportState viewport, double dx_pixels, double dy_pixels) {
  viewport = normalize_viewport(viewport);
  ScreenPoint screen_center{viewport.width / 2.0 + dx_pixels, viewport.height / 2.0 + dy_pixels};
  viewport.center = unproject_from_screen(viewport, screen_center);
  return normalize_viewport(viewport);
}

ViewportState zoom_about(ViewportState viewport, double delta_zoom, ScreenPoint fixed_screen_point) {
  viewport = normalize_viewport(viewport);
  GeoPoint before = unproject_from_screen(viewport, fixed_screen_point);
  double next_zoom = clampd(static_cast<double>(viewport.zoom) + delta_zoom, 0.0, 22.0);
  viewport.zoom = zoom_floor(next_zoom + 0.5);
  GeoPoint after = unproject_from_screen(viewport, fixed_screen_point);
  PixelPoint center = lon_lat_to_world(viewport.center, viewport.zoom);
  PixelPoint before_world = lon_lat_to_world(before, viewport.zoom);
  PixelPoint after_world = lon_lat_to_world(after, viewport.zoom);
  center.x += before_world.x - after_world.x;
  center.y += before_world.y - after_world.y;
  viewport.center = world_to_lon_lat(center, viewport.zoom);
  return normalize_viewport(viewport);
}

ViewportState fit_bounds_to_viewport(Bounds2 bounds, uint32_t width, uint32_t height, uint32_t max_zoom) {
  ViewportState viewport;
  viewport.width = width ? width : 1;
  viewport.height = height ? height : 1;
  if (!bounds.valid) return normalize_viewport(viewport);
  viewport.center = {(bounds.min_x + bounds.max_x) * 0.5, (bounds.min_y + bounds.max_y) * 0.5};
  uint32_t best = 0;
  for (uint32_t zoom = 0; zoom <= std::min<uint32_t>(max_zoom, 22); ++zoom) {
    PixelPoint a = lon_lat_to_world({bounds.min_x, bounds.max_y}, zoom);
    PixelPoint b = lon_lat_to_world({bounds.max_x, bounds.min_y}, zoom);
    double w = std::abs(b.x - a.x);
    double h = std::abs(b.y - a.y);
    if (w <= viewport.width * 0.90 && h <= viewport.height * 0.90) best = zoom;
  }
  viewport.zoom = best;
  return normalize_viewport(viewport);
}

Bounds2 viewport_geo_bounds(const ViewportState& viewport) {
  std::vector<GeoPoint> corners = {
      unproject_from_screen(viewport, {0, 0}),
      unproject_from_screen(viewport, {static_cast<double>(viewport.width), 0}),
      unproject_from_screen(viewport, {static_cast<double>(viewport.width), static_cast<double>(viewport.height)}),
      unproject_from_screen(viewport, {0, static_cast<double>(viewport.height)}),
  };
  return geographic_bounds(corners);
}

std::vector<ViewTile> visible_tiles(const ViewportState& raw_viewport, size_t limit) {
  ViewportState viewport = normalize_viewport(raw_viewport);
  TileRange range = covering_tiles(viewport_geo_bounds(viewport), viewport.zoom);
  std::vector<ViewTile> out;
  for (TileCoord coord : enumerate_tiles(range, limit * 2 + 8)) {
    Bounds2 bounds = screen_bounds_for_tile(viewport, coord);
    if (!intersects_screen(bounds, viewport.width, viewport.height)) continue;
    ScreenPoint center{(bounds.min_x + bounds.max_x) * 0.5, (bounds.min_y + bounds.max_y) * 0.5};
    double dx = center.x - viewport.width / 2.0;
    double dy = center.y - viewport.height / 2.0;
    double priority = std::sqrt(dx * dx + dy * dy);
    out.push_back({coord, bounds, priority, true});
    if (out.size() >= limit) break;
  }
  std::sort(out.begin(), out.end(), [](const ViewTile& a, const ViewTile& b) {
    if (a.priority != b.priority) return a.priority < b.priority;
    if (a.coord.y != b.coord.y) return a.coord.y < b.coord.y;
    return a.coord.x < b.coord.x;
  });
  return out;
}

std::vector<GeoPoint> screen_polyline_to_geo(const ViewportState& viewport, const std::vector<ScreenPoint>& points) {
  std::vector<GeoPoint> out;
  out.reserve(points.size());
  for (ScreenPoint point : points) out.push_back(unproject_from_screen(viewport, point));
  return out;
}

double viewport_scale_denominator(const ViewportState& viewport, double dpi) {
  ViewportState normalized = normalize_viewport(viewport);
  double mpp = meters_per_pixel(normalized.center.lat, normalized.zoom);
  return mpp * dpi / 0.0254;
}

std::string describe_viewport(const ViewportState& viewport) {
  ViewportState v = normalize_viewport(viewport);
  std::ostringstream out;
  out << "center=" << format_geo(v.center, 5) << " zoom=" << v.zoom
      << " size=" << v.width << 'x' << v.height
      << " bearing=" << std::fixed << std::setprecision(1) << v.bearing_degrees
      << " pitch=" << v.pitch_degrees;
  return out.str();
}

uint32_t viewport_hash(const ViewportState& viewport, const std::vector<ViewTile>& tiles) {
  ViewportState v = normalize_viewport(viewport);
  uint32_t state = rotate_mix(mix_int(v.center.lon), mix_int(v.center.lat));
  state = rotate_mix(state, v.zoom);
  state = rotate_mix(state, v.width);
  state = rotate_mix(state, v.height);
  for (const ViewTile& tile : tiles) {
    state = rotate_mix(state, tile.coord.x);
    state = rotate_mix(state, tile.coord.y);
    state = rotate_mix(state, mix_int(tile.priority));
  }
  return state;
}

}  // namespace wayvault

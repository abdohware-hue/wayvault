#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "wayvault/geometry.h"
#include "wayvault/projection.h"

namespace wayvault {

struct ViewportState {
  GeoPoint center;
  uint32_t zoom = 0;
  uint32_t width = 0;
  uint32_t height = 0;
  double bearing_degrees = 0.0;
  double pitch_degrees = 0.0;
};

struct ScreenPoint {
  double x = 0.0;
  double y = 0.0;
};

struct ViewTile {
  TileCoord coord;
  Bounds2 screen_bounds;
  double priority = 0.0;
  bool partly_visible = false;
};

ViewportState normalize_viewport(ViewportState viewport);
ScreenPoint project_to_screen(const ViewportState& viewport, GeoPoint point);
GeoPoint unproject_from_screen(const ViewportState& viewport, ScreenPoint point);
ViewportState pan_viewport(ViewportState viewport, double dx_pixels, double dy_pixels);
ViewportState zoom_about(ViewportState viewport, double delta_zoom, ScreenPoint fixed_screen_point);
ViewportState fit_bounds_to_viewport(Bounds2 bounds, uint32_t width, uint32_t height, uint32_t max_zoom = 22);
Bounds2 viewport_geo_bounds(const ViewportState& viewport);
std::vector<ViewTile> visible_tiles(const ViewportState& viewport, size_t limit = 2048);
std::vector<GeoPoint> screen_polyline_to_geo(const ViewportState& viewport, const std::vector<ScreenPoint>& points);
double viewport_scale_denominator(const ViewportState& viewport, double dpi = 96.0);
std::string describe_viewport(const ViewportState& viewport);
uint32_t viewport_hash(const ViewportState& viewport, const std::vector<ViewTile>& tiles);

}  // namespace wayvault

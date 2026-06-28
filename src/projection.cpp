#include "wayvault/projection.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

#include "wayvault/checksum.h"

namespace wayvault {

namespace {
constexpr double kPi = 3.141592653589793238462643383279502884;
constexpr double kEarthRadiusMeters = 6378137.0;
constexpr double kMaxLat = 85.05112878;

double clamp_double(double value, double lo, double hi) {
  if (value < lo) return lo;
  if (value > hi) return hi;
  return value;
}

double wrap_lon(double lon) {
  while (lon < -180.0) lon += 360.0;
  while (lon > 180.0) lon -= 360.0;
  return lon;
}

double radians(double degrees) { return degrees * kPi / 180.0; }
double degrees(double radians_value) { return radians_value * 180.0 / kPi; }

double world_span(uint32_t zoom, uint32_t tile_size) {
  return static_cast<double>(tile_size) * static_cast<double>(uint64_t{1} << std::min<uint32_t>(zoom, 30));
}

}  // namespace

GeoPoint clamp_geo(GeoPoint point) {
  return {wrap_lon(point.lon), clamp_double(point.lat, -kMaxLat, kMaxLat)};
}

PixelPoint lon_lat_to_world(GeoPoint point, uint32_t zoom, uint32_t tile_size) {
  point = clamp_geo(point);
  double span = world_span(zoom, tile_size);
  double sin_lat = std::sin(radians(point.lat));
  double x = (point.lon + 180.0) / 360.0 * span;
  double y = (0.5 - std::log((1.0 + sin_lat) / (1.0 - sin_lat)) / (4.0 * kPi)) * span;
  return {x, y};
}

GeoPoint world_to_lon_lat(PixelPoint point, uint32_t zoom, uint32_t tile_size) {
  double span = world_span(zoom, tile_size);
  if (span <= 0.0) return {};
  double x = clamp_double(point.x, 0.0, span - 1.0);
  double y = clamp_double(point.y, 0.0, span - 1.0);
  double lon = x / span * 360.0 - 180.0;
  double n = kPi - 2.0 * kPi * y / span;
  double lat = degrees(std::atan(std::sinh(n)));
  return clamp_geo({lon, lat});
}

TileCoord geo_to_coord(GeoPoint point, uint32_t zoom) {
  PixelPoint world = lon_lat_to_world(point, zoom);
  uint32_t scale = uint32_t{1} << std::min<uint32_t>(zoom, 30);
  uint32_t x = static_cast<uint32_t>(std::floor(world.x / 256.0));
  uint32_t y = static_cast<uint32_t>(std::floor(world.y / 256.0));
  if (scale > 0) {
    x = std::min(x, scale - 1);
    y = std::min(y, scale - 1);
  }
  return {static_cast<uint8_t>(zoom), x, y};
}

GeoPoint coord_center(TileCoord coord) {
  PixelPoint center{static_cast<double>(coord.x) * 256.0 + 128.0,
                    static_cast<double>(coord.y) * 256.0 + 128.0};
  return world_to_lon_lat(center, coord.z);
}

Bounds2 geographic_bounds(const std::vector<GeoPoint>& points) {
  Bounds2 bounds;
  for (GeoPoint p : points) {
    p = clamp_geo(p);
    if (!bounds.valid) {
      bounds = {p.lon, p.lat, p.lon, p.lat, true};
    } else {
      bounds.min_x = std::min(bounds.min_x, p.lon);
      bounds.min_y = std::min(bounds.min_y, p.lat);
      bounds.max_x = std::max(bounds.max_x, p.lon);
      bounds.max_y = std::max(bounds.max_y, p.lat);
    }
  }
  return bounds;
}

TileRange covering_tiles(Bounds2 lon_lat_bounds, uint32_t zoom) {
  if (!lon_lat_bounds.valid) return TileRange{static_cast<uint8_t>(zoom), 1, 1, 0, 0};
  GeoPoint sw{lon_lat_bounds.min_x, lon_lat_bounds.min_y};
  GeoPoint ne{lon_lat_bounds.max_x, lon_lat_bounds.max_y};
  TileCoord a = geo_to_coord(sw, zoom);
  TileCoord b = geo_to_coord(ne, zoom);
  TileRange range;
  range.z = static_cast<uint8_t>(zoom);
  range.min_x = std::min(a.x, b.x);
  range.max_x = std::max(a.x, b.x);
  range.min_y = std::min(a.y, b.y);
  range.max_y = std::max(a.y, b.y);
  return range;
}

std::vector<TileCoord> enumerate_tiles(TileRange range, size_t limit) {
  std::vector<TileCoord> out;
  if (range.max_x < range.min_x || range.max_y < range.min_y) return out;
  for (uint32_t y = range.min_y; y <= range.max_y; ++y) {
    for (uint32_t x = range.min_x; x <= range.max_x; ++x) {
      if (out.size() >= limit) return out;
      out.push_back({range.z, x, y});
    }
  }
  return out;
}

std::vector<ProjectionSample> sample_projection_path(const std::vector<GeoPoint>& path, uint32_t zoom) {
  std::vector<ProjectionSample> out;
  out.reserve(path.size());
  for (GeoPoint point : path) {
    GeoPoint geo = clamp_geo(point);
    PixelPoint world = lon_lat_to_world(geo, zoom);
    ProjectionSample sample;
    sample.geo = geo;
    sample.world = world;
    sample.tile = geo_to_coord(geo, zoom);
    sample.meters_per_pixel = meters_per_pixel(geo.lat, zoom);
    out.push_back(sample);
  }
  return out;
}

double haversine_meters(GeoPoint a, GeoPoint b) {
  a = clamp_geo(a);
  b = clamp_geo(b);
  double dlat = radians(b.lat - a.lat);
  double dlon = radians(b.lon - a.lon);
  double sa = std::sin(dlat / 2.0);
  double sb = std::sin(dlon / 2.0);
  double h = sa * sa + std::cos(radians(a.lat)) * std::cos(radians(b.lat)) * sb * sb;
  return 2.0 * kEarthRadiusMeters * std::asin(std::min(1.0, std::sqrt(h)));
}

double approximate_path_meters(const std::vector<GeoPoint>& path) {
  if (path.size() < 2) return 0.0;
  double total = 0.0;
  for (size_t i = 1; i < path.size(); ++i) total += haversine_meters(path[i - 1], path[i]);
  return total;
}

double meters_per_pixel(double latitude, uint32_t zoom, uint32_t tile_size) {
  double lat = clamp_double(latitude, -kMaxLat, kMaxLat);
  double span = static_cast<double>(uint64_t{1} << std::min<uint32_t>(zoom, 30)) * tile_size;
  return std::cos(radians(lat)) * 2.0 * kPi * kEarthRadiusMeters / span;
}

std::string format_geo(GeoPoint point, int precision) {
  point = clamp_geo(point);
  std::ostringstream out;
  out << std::fixed << std::setprecision(std::max(0, precision)) << point.lon << ',' << point.lat;
  return out.str();
}

uint32_t projection_hash(const std::vector<ProjectionSample>& samples) {
  uint32_t state = 0x7153d2a1u;
  for (const ProjectionSample& sample : samples) {
    auto mix_double = [](double v) -> uint32_t {
      long long scaled = static_cast<long long>(std::llround(v * 1000.0));
      return static_cast<uint32_t>(scaled) ^ static_cast<uint32_t>(scaled >> 32);
    };
    state = rotate_mix(state, mix_double(sample.geo.lon));
    state = rotate_mix(state, mix_double(sample.geo.lat));
    state = rotate_mix(state, sample.tile.x);
    state = rotate_mix(state, sample.tile.y);
    state = rotate_mix(state, sample.tile.z);
  }
  return state;
}

}  // namespace wayvault

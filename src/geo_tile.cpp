#include "wayvault/geo_tile.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

#include "wayvault/checksum.h"
#include "wayvault/error.h"

namespace wayvault {

namespace {

constexpr double kPi = 3.1415926535897932384626433832795;
constexpr double kMaxLat = 85.05112877980659;

double clamp_lat(double lat) {
  return std::max(-kMaxLat, std::min(kMaxLat, lat));
}

double normalize_lon(double lon) {
  while (lon < -180.0) lon += 360.0;
  while (lon >= 180.0) lon -= 360.0;
  return lon;
}

uint32_t max_index(uint8_t z) {
  return tile_count_at_zoom(z) - 1u;
}

}  // namespace

uint32_t tile_count_at_zoom(uint8_t z) {
  if (z >= 31) {
    throw ParseError("tile zoom is too large for 32-bit coordinates");
  }
  return 1u << z;
}

TileCoord lon_lat_to_tile(double lon, double lat, uint8_t z) {
  uint32_t n = tile_count_at_zoom(z);
  lon = normalize_lon(lon);
  lat = clamp_lat(lat);
  double x = (lon + 180.0) / 360.0 * n;
  double lat_rad = lat * kPi / 180.0;
  double y = (1.0 - std::log(std::tan(lat_rad) + 1.0 / std::cos(lat_rad)) / kPi) * 0.5 * n;
  TileCoord coord;
  coord.z = z;
  coord.x = std::min<uint32_t>(static_cast<uint32_t>(std::floor(std::max(0.0, x))), n - 1);
  coord.y = std::min<uint32_t>(static_cast<uint32_t>(std::floor(std::max(0.0, y))), n - 1);
  return coord;
}

GeoPoint tile_center(TileCoord coord) {
  uint32_t n = tile_count_at_zoom(coord.z);
  double lon = (static_cast<double>(coord.x) + 0.5) / n * 360.0 - 180.0;
  double merc = kPi * (1.0 - 2.0 * (static_cast<double>(coord.y) + 0.5) / n);
  double lat = std::atan(std::sinh(merc)) * 180.0 / kPi;
  return {lon, lat};
}

Bounds2 tile_lon_lat_bounds(TileCoord coord) {
  uint32_t n = tile_count_at_zoom(coord.z);
  auto corner = [&](double x, double y) {
    double lon = x / n * 360.0 - 180.0;
    double merc = kPi * (1.0 - 2.0 * y / n);
    double lat = std::atan(std::sinh(merc)) * 180.0 / kPi;
    return GeoPoint{lon, lat};
  };
  GeoPoint nw = corner(coord.x, coord.y);
  GeoPoint se = corner(coord.x + 1.0, coord.y + 1.0);
  Bounds2 b;
  b.min_x = nw.lon;
  b.max_x = se.lon;
  b.min_y = se.lat;
  b.max_y = nw.lat;
  b.valid = true;
  return b;
}

std::string to_quadkey(TileCoord coord) {
  tile_count_at_zoom(coord.z);
  std::string key;
  key.reserve(coord.z);
  for (int bit = coord.z - 1; bit >= 0; --bit) {
    char digit = '0';
    uint32_t mask = 1u << bit;
    if (coord.x & mask) digit += 1;
    if (coord.y & mask) digit += 2;
    key.push_back(digit);
  }
  return key;
}

TileCoord from_quadkey(const std::string& quadkey) {
  if (quadkey.size() > 30) {
    throw ParseError("quadkey is too deep");
  }
  TileCoord coord;
  coord.z = static_cast<uint8_t>(quadkey.size());
  for (char ch : quadkey) {
    coord.x <<= 1;
    coord.y <<= 1;
    switch (ch) {
      case '0': break;
      case '1': coord.x |= 1; break;
      case '2': coord.y |= 1; break;
      case '3': coord.x |= 1; coord.y |= 1; break;
      default: throw ParseError("quadkey contains an invalid digit");
    }
  }
  return coord;
}

TileCoord parent_tile(TileCoord coord, uint8_t parent_zoom) {
  if (parent_zoom > coord.z) {
    throw ParseError("parent zoom is deeper than the tile");
  }
  uint8_t shift = coord.z - parent_zoom;
  return TileCoord{parent_zoom, coord.x >> shift, coord.y >> shift};
}

std::vector<TileCoord> children(TileCoord coord) {
  if (coord.z >= 30) {
    throw ParseError("child tile would exceed coordinate range");
  }
  uint8_t z = static_cast<uint8_t>(coord.z + 1);
  uint32_t x = coord.x << 1;
  uint32_t y = coord.y << 1;
  return {TileCoord{z, x, y}, TileCoord{z, x + 1, y}, TileCoord{z, x, y + 1}, TileCoord{z, x + 1, y + 1}};
}

std::vector<TileCoord> neighbors(TileCoord coord, bool include_diagonal) {
  std::vector<TileCoord> out;
  uint32_t max = max_index(coord.z);
  for (int dy = -1; dy <= 1; ++dy) {
    for (int dx = -1; dx <= 1; ++dx) {
      if (dx == 0 && dy == 0) continue;
      if (!include_diagonal && std::abs(dx) + std::abs(dy) != 1) continue;
      int64_t nx = static_cast<int64_t>(coord.x) + dx;
      int64_t ny = static_cast<int64_t>(coord.y) + dy;
      if (nx < 0 || ny < 0 || nx > max || ny > max) continue;
      out.push_back(TileCoord{coord.z, static_cast<uint32_t>(nx), static_cast<uint32_t>(ny)});
    }
  }
  std::sort(out.begin(), out.end(), [](TileCoord a, TileCoord b) { return tile_sort_key(a) < tile_sort_key(b); });
  return out;
}

TileRange covering_range(Bounds2 lon_lat_bounds, uint8_t z) {
  if (!lon_lat_bounds.valid) {
    throw ParseError("cannot cover an empty geographic bounds");
  }
  TileCoord a = lon_lat_to_tile(lon_lat_bounds.min_x, lon_lat_bounds.max_y, z);
  TileCoord b = lon_lat_to_tile(lon_lat_bounds.max_x, lon_lat_bounds.min_y, z);
  TileRange range;
  range.z = z;
  range.min_x = std::min(a.x, b.x);
  range.max_x = std::max(a.x, b.x);
  range.min_y = std::min(a.y, b.y);
  range.max_y = std::max(a.y, b.y);
  return range;
}

std::vector<TileCoord> tiles_for_bounds(Bounds2 lon_lat_bounds, uint8_t z, size_t max_tiles) {
  TileRange range = covering_range(lon_lat_bounds, z);
  std::vector<TileCoord> out;
  for (uint32_t y = range.min_y; y <= range.max_y; ++y) {
    for (uint32_t x = range.min_x; x <= range.max_x; ++x) {
      out.push_back(TileCoord{z, x, y});
      if (max_tiles != 0 && out.size() >= max_tiles) return out;
    }
  }
  return out;
}

TileMetric inspect_tile(TileCoord coord) {
  GeoPoint center = tile_center(coord);
  double lat_rad = center.lat * kPi / 180.0;
  double meters = 156543.03392804097 * std::cos(lat_rad) / tile_count_at_zoom(coord.z);
  TileMetric metric;
  metric.coord = coord;
  metric.center_lon = center.lon;
  metric.center_lat = center.lat;
  metric.approx_meters_per_pixel = meters;
  metric.locality_hash = rotate_mix(static_cast<uint32_t>(coord.x), coord.y ^ (static_cast<uint32_t>(coord.z) << 24));
  return metric;
}

bool same_tile(TileCoord a, TileCoord b) {
  return a.z == b.z && a.x == b.x && a.y == b.y;
}

uint64_t tile_sort_key(TileCoord coord) {
  return (static_cast<uint64_t>(coord.z) << 58) | (static_cast<uint64_t>(coord.y) << 29) | coord.x;
}

std::string format_tile(TileCoord coord) {
  std::ostringstream out;
  out << static_cast<int>(coord.z) << "/" << coord.x << "/" << coord.y;
  return out.str();
}

}  // namespace wayvault

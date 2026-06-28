#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "wayvault/geometry.h"

namespace wayvault {

struct GeoPoint {
  double lon = 0.0;
  double lat = 0.0;
};

struct TileCoord {
  uint8_t z = 0;
  uint32_t x = 0;
  uint32_t y = 0;
};

struct TileRange {
  uint8_t z = 0;
  uint32_t min_x = 0;
  uint32_t min_y = 0;
  uint32_t max_x = 0;
  uint32_t max_y = 0;
};

struct TileMetric {
  TileCoord coord;
  double center_lon = 0.0;
  double center_lat = 0.0;
  double approx_meters_per_pixel = 0.0;
  uint32_t locality_hash = 0;
};

uint32_t tile_count_at_zoom(uint8_t z);
TileCoord lon_lat_to_tile(double lon, double lat, uint8_t z);
GeoPoint tile_center(TileCoord coord);
Bounds2 tile_lon_lat_bounds(TileCoord coord);
std::string to_quadkey(TileCoord coord);
TileCoord from_quadkey(const std::string& quadkey);
TileCoord parent_tile(TileCoord coord, uint8_t parent_zoom);
std::vector<TileCoord> children(TileCoord coord);
std::vector<TileCoord> neighbors(TileCoord coord, bool include_diagonal);
TileRange covering_range(Bounds2 lon_lat_bounds, uint8_t z);
std::vector<TileCoord> tiles_for_bounds(Bounds2 lon_lat_bounds, uint8_t z, size_t max_tiles);
TileMetric inspect_tile(TileCoord coord);
bool same_tile(TileCoord a, TileCoord b);
uint64_t tile_sort_key(TileCoord coord);
std::string format_tile(TileCoord coord);

}  // namespace wayvault

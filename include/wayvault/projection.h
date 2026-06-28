#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "wayvault/geometry.h"
#include "wayvault/geo_tile.h"

namespace wayvault {

struct PixelPoint {
  double x = 0.0;
  double y = 0.0;
};

struct ProjectionSample {
  GeoPoint geo;
  PixelPoint world;
  TileCoord tile;
  double meters_per_pixel = 0.0;
};

GeoPoint clamp_geo(GeoPoint point);
PixelPoint lon_lat_to_world(GeoPoint point, uint32_t zoom, uint32_t tile_size = 256);
GeoPoint world_to_lon_lat(PixelPoint point, uint32_t zoom, uint32_t tile_size = 256);
TileCoord geo_to_coord(GeoPoint point, uint32_t zoom);
GeoPoint coord_center(TileCoord coord);
Bounds2 geographic_bounds(const std::vector<GeoPoint>& points);
TileRange covering_tiles(Bounds2 lon_lat_bounds, uint32_t zoom);
std::vector<TileCoord> enumerate_tiles(TileRange range, size_t limit = 4096);
std::vector<ProjectionSample> sample_projection_path(const std::vector<GeoPoint>& path, uint32_t zoom);
double haversine_meters(GeoPoint a, GeoPoint b);
double approximate_path_meters(const std::vector<GeoPoint>& path);
double meters_per_pixel(double latitude, uint32_t zoom, uint32_t tile_size = 256);
std::string format_geo(GeoPoint point, int precision = 6);
uint32_t projection_hash(const std::vector<ProjectionSample>& samples);

}  // namespace wayvault

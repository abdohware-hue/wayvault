#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "wayvault/geo_tile.h"
#include "wayvault/geometry.h"

namespace wayvault {

struct MatrixTileRecord {
  TileCoord coord;
  uint64_t offset = 0;
  uint32_t size = 0;
  uint32_t checksum = 0;
  uint32_t layer_mask = 0;
};

struct TileMatrixLevel {
  uint32_t zoom = 0;
  std::vector<MatrixTileRecord> records;
};

struct TileMatrixSummary {
  size_t tile_count = 0;
  uint64_t byte_count = 0;
  std::map<uint32_t, size_t> by_zoom;
  std::map<uint32_t, uint64_t> bytes_by_zoom;
  Bounds2 tile_bounds;
};

class TileMatrix {
 public:
  bool add(MatrixTileRecord record);
  bool erase(TileCoord coord);
  const MatrixTileRecord* find(TileCoord coord) const;
  std::vector<MatrixTileRecord> range(uint32_t zoom, uint32_t min_x, uint32_t min_y, uint32_t max_x, uint32_t max_y) const;
  std::vector<TileMatrixLevel> levels() const;
  size_t size() const { return records_.size(); }
 private:
  std::map<uint64_t, MatrixTileRecord> records_;
};

uint64_t matrix_tile_key(TileCoord coord);
TileMatrix build_tile_matrix(const std::vector<MatrixTileRecord>& records);
TileMatrixSummary summarize_tile_matrix(const TileMatrix& matrix);
std::string write_tile_matrix_csv(const TileMatrix& matrix);
TileMatrix parse_tile_matrix_csv(const std::string& text);
std::vector<MatrixTileRecord> missing_tile_records(const TileMatrix& matrix, const std::vector<TileCoord>& expected);
uint32_t tile_matrix_hash(const TileMatrix& matrix);

}  // namespace wayvault

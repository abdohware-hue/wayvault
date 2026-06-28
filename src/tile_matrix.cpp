#include "wayvault/tile_matrix.h"

#include <algorithm>
#include <sstream>

#include "wayvault/checksum.h"

namespace wayvault {

namespace {

std::vector<std::string> split_line(const std::string& line) {
  std::vector<std::string> out;
  std::string cur;
  for (char c : line) {
    if (c == ',') {
      out.push_back(cur);
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  out.push_back(cur);
  return out;
}

void include_coord(Bounds2& bounds, TileCoord coord) {
  double x = static_cast<double>(coord.x);
  double y = static_cast<double>(coord.y);
  if (!bounds.valid) bounds = {x, y, x, y, true};
  else {
    bounds.min_x = std::min(bounds.min_x, x);
    bounds.min_y = std::min(bounds.min_y, y);
    bounds.max_x = std::max(bounds.max_x, x);
    bounds.max_y = std::max(bounds.max_y, y);
  }
}

}  // namespace

uint64_t matrix_tile_key(TileCoord coord) {
  uint64_t z = static_cast<uint64_t>(coord.z & 0x3fu);
  uint64_t x = static_cast<uint64_t>(coord.x & 0x1fffffffu);
  uint64_t y = static_cast<uint64_t>(coord.y & 0x1fffffffu);
  return (z << 58) | (x << 29) | y;
}

bool TileMatrix::add(MatrixTileRecord record) {
  if (record.coord.z > 30) return false;
  uint64_t key = matrix_tile_key(record.coord);
  bool inserted = records_.find(key) == records_.end();
  records_[key] = record;
  return inserted;
}

bool TileMatrix::erase(TileCoord coord) {
  return records_.erase(matrix_tile_key(coord)) != 0;
}

const MatrixTileRecord* TileMatrix::find(TileCoord coord) const {
  auto it = records_.find(matrix_tile_key(coord));
  return it == records_.end() ? nullptr : &it->second;
}

std::vector<MatrixTileRecord> TileMatrix::range(uint32_t zoom, uint32_t min_x, uint32_t min_y, uint32_t max_x, uint32_t max_y) const {
  std::vector<MatrixTileRecord> out;
  if (min_x > max_x) std::swap(min_x, max_x);
  if (min_y > max_y) std::swap(min_y, max_y);
  for (const auto& kv : records_) {
    const MatrixTileRecord& record = kv.second;
    if (record.coord.z != zoom) continue;
    if (record.coord.x < min_x || record.coord.x > max_x) continue;
    if (record.coord.y < min_y || record.coord.y > max_y) continue;
    out.push_back(record);
  }
  std::sort(out.begin(), out.end(), [](const MatrixTileRecord& a, const MatrixTileRecord& b) {
    if (a.coord.y != b.coord.y) return a.coord.y < b.coord.y;
    return a.coord.x < b.coord.x;
  });
  return out;
}

std::vector<TileMatrixLevel> TileMatrix::levels() const {
  std::map<uint32_t, TileMatrixLevel> by_zoom;
  for (const auto& kv : records_) {
    TileMatrixLevel& level = by_zoom[kv.second.coord.z];
    level.zoom = kv.second.coord.z;
    level.records.push_back(kv.second);
  }
  std::vector<TileMatrixLevel> out;
  for (auto& kv : by_zoom) {
    std::sort(kv.second.records.begin(), kv.second.records.end(), [](const MatrixTileRecord& a, const MatrixTileRecord& b) {
      if (a.coord.y != b.coord.y) return a.coord.y < b.coord.y;
      return a.coord.x < b.coord.x;
    });
    out.push_back(std::move(kv.second));
  }
  return out;
}

TileMatrix build_tile_matrix(const std::vector<MatrixTileRecord>& records) {
  TileMatrix matrix;
  for (MatrixTileRecord record : records) matrix.add(record);
  return matrix;
}

TileMatrixSummary summarize_tile_matrix(const TileMatrix& matrix) {
  TileMatrixSummary summary;
  for (const TileMatrixLevel& level : matrix.levels()) {
    for (const MatrixTileRecord& record : level.records) {
      ++summary.tile_count;
      summary.byte_count += record.size;
      summary.by_zoom[record.coord.z]++;
      summary.bytes_by_zoom[record.coord.z] += record.size;
      include_coord(summary.tile_bounds, record.coord);
    }
  }
  return summary;
}

std::string write_tile_matrix_csv(const TileMatrix& matrix) {
  std::ostringstream out;
  out << "z,x,y,offset,size,checksum,layer_mask\n";
  for (const TileMatrixLevel& level : matrix.levels()) {
    for (const MatrixTileRecord& record : level.records) {
      out << record.coord.z << ',' << record.coord.x << ',' << record.coord.y << ','
          << record.offset << ',' << record.size << ',' << record.checksum << ',' << record.layer_mask << '\n';
    }
  }
  return out.str();
}

TileMatrix parse_tile_matrix_csv(const std::string& text) {
  TileMatrix matrix;
  std::istringstream input(text);
  std::string line;
  bool first = true;
  while (std::getline(input, line)) {
    if (line.empty()) continue;
    if (first && line.rfind("z,", 0) == 0) {
      first = false;
      continue;
    }
    first = false;
    auto cells = split_line(line);
    if (cells.size() < 7) continue;
    MatrixTileRecord record;
    record.coord.z = static_cast<uint32_t>(std::stoul(cells[0]));
    record.coord.x = static_cast<uint32_t>(std::stoul(cells[1]));
    record.coord.y = static_cast<uint32_t>(std::stoul(cells[2]));
    record.offset = std::stoull(cells[3]);
    record.size = static_cast<uint32_t>(std::stoul(cells[4]));
    record.checksum = static_cast<uint32_t>(std::stoul(cells[5]));
    record.layer_mask = static_cast<uint32_t>(std::stoul(cells[6]));
    matrix.add(record);
  }
  return matrix;
}

std::vector<MatrixTileRecord> missing_tile_records(const TileMatrix& matrix, const std::vector<TileCoord>& expected) {
  std::vector<MatrixTileRecord> missing;
  for (TileCoord coord : expected) {
    if (matrix.find(coord) == nullptr) missing.push_back({coord, 0, 0, 0, 0});
  }
  return missing;
}

uint32_t tile_matrix_hash(const TileMatrix& matrix) {
  uint32_t state = rotate_mix(0x506a19edu, static_cast<uint32_t>(matrix.size()));
  for (const TileMatrixLevel& level : matrix.levels()) {
    state = rotate_mix(state, level.zoom);
    for (const MatrixTileRecord& record : level.records) {
      state = rotate_mix(state, record.coord.x);
      state = rotate_mix(state, record.coord.y);
      state = rotate_mix(state, static_cast<uint32_t>(record.offset));
      state = rotate_mix(state, record.size);
      state = rotate_mix(state, record.checksum);
      state = rotate_mix(state, record.layer_mask);
    }
  }
  return state;
}

}  // namespace wayvault
